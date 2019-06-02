#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "utils.h"

int un_sock;
int web_sock;
int epoll;
char *un_path;

uint64_t id;

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
client_t clients[MAX_CLIENTS];

pthread_t pinger;
pthread_t commander;

void init_server(char*, char*);

void *commander_start_routine(void*);
void *pinger_start_routine(void*);

void handle_call(int);

void send_msg(int sock, sock_msg_t msg);
void send_empty(int, sock_msg_type_t);

void del_client(int);

int client_by_name(char*);

sock_msg_t get_msg(int, struct sockaddr*, socklen_t*);
void del_msg(sock_msg_t);

void int_handler(int);
void cleanup(void);

int main(int argc, char *argv[]) {
    if (argc != 3) die("Pass: udp port, unix socket path");

    init_server(argv[1], argv[2]);

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) == -1) die_errno();
        handle_call(event.data.fd);
    }
}

void init_server(char* port, char* unix_path)
{
    // register atexit
    if (atexit(cleanup) == -1) die_errno();

    // register int handler
    if (signal(SIGINT, int_handler) == SIG_ERR)
        show_errno();

    // parse port
    uint16_t port_num = (uint16_t) parse_pos_int(port);
    if (port_num < 1024)
        die("Invalid port number");

    // parse unix path
    un_path = unix_path;
    if (strlen(un_path) < 1 || strlen(un_path) > UNIX_PATH_MAX)
        die("Invalid unix socket path");

    // init web socket
    struct sockaddr_in web_addr;
    memset(&web_addr, 0, sizeof(struct sockaddr_in));
    web_addr.sin_family = AF_INET;
    web_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    web_addr.sin_port = htons(port_num);

    if ((web_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        die_errno();

    if (bind(web_sock, (const struct sockaddr *) &web_addr, sizeof(web_addr)))
        die_errno();

    // init unix socket
    struct sockaddr_un un_addr;
    un_addr.sun_family = AF_UNIX;

    snprintf(un_addr.sun_path, UNIX_PATH_MAX, "%s", un_path);

    if ((un_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        die_errno();

    if (bind(un_sock, (const struct sockaddr *) &un_addr, sizeof(un_addr)))
        die_errno();

    // init epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) == -1)
        die_errno();

    event.data.fd = web_sock;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, web_sock, &event) == -1)
        die_errno();

    event.data.fd = un_sock;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, un_sock, &event) == -1)
        die_errno();

    // start threads
    if (pthread_create(&commander, NULL, commander_start_routine, NULL) != 0)
        die_errno();
    if (pthread_detach(commander) != 0)
        die_errno();

    if (pthread_create(&pinger, NULL, pinger_start_routine, NULL) != 0)
        die_errno();
    if (pthread_detach(pinger) != 0)
        die_errno();
}

void *commander_start_routine(void *params)
{
    char buffer[1024];
    while (1) {
        int min_i = MAX_CLIENTS;
        int min = 1000000;

        // read command
        scanf("%1023s", buffer);

        // open file
        FILE *file = fopen(buffer, "r");
        if (file == NULL) {
            show_errno();
            continue;
        }
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        char *file_buff = malloc(size + 1);
        if (file_buff == NULL) {
            show_errno();
            continue;
        }

        file_buff[size] = '\0';

        if (fread(file_buff, 1, size, file) != size) {
            fprintf(stderr, "Could not read file\n");
            free(file_buff);
            continue;
        }

        fclose(file);

        // send request
        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].fd) continue;
            if (min > clients[i].working) {
                min_i = i;
                min = clients[i].working;
            }
        }

        if (min_i < MAX_CLIENTS) {
            sock_msg_t msg = { WORK, strlen(file_buff) + 1, 0, ++id, file_buff, NULL };
            printf("JOB %lu SEND TO %s\n", id, clients[min_i].name);
            send_msg(min_i, msg);
            clients[min_i].working++;
        } else {
            fprintf(stderr, "No clients connected\n");
        }
        pthread_mutex_unlock(&client_mutex);

        free(file_buff);
    }
}

void *pinger_start_routine(void *params)
{
    while (1) {
        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd == 0) continue;
            if (clients[i].inactive) {
                del_client(i);
            } else {
                clients[i].inactive = 1;
                send_empty(i, PING);
            }
        }
        pthread_mutex_unlock(&client_mutex);
        sleep(10);
    }
}

void handle_call(int sock)
{
    struct sockaddr *addr = malloc(sizeof(struct sockaddr));
    if (addr == NULL) die_errno();
    socklen_t addr_len = sizeof(struct sockaddr);
    sock_msg_t msg = get_msg(sock, addr, &addr_len);
    pthread_mutex_lock(&client_mutex);

    switch (msg.type) {
    case REGISTER: {
        sock_msg_type_t reply = OK;
        int i;
        i = client_by_name(msg.name);
        if (i < MAX_CLIENTS)
            reply = NAME_TAKEN;

        for (i = 0; i < MAX_CLIENTS && clients[i].fd != 0; i++);

        if (i == MAX_CLIENTS)
            reply = FULL;

        if (reply != OK) {
            send_empty(sock, reply);
            break;
        }

        clients[i].fd = sock;
        clients[i].name = malloc(msg.name_size + 1);
        if (clients[i].name == NULL) die_errno();
        strcpy(clients[i].name, msg.name);
        clients[i].addr = addr;
        clients[i].addr_len = addr_len;
        clients[i].working = 0;
        clients[i].inactive = 0;

        send_empty(i, OK);
        break;
    }
    case UNREGISTER: {
        int i;
        for (i = 0; i < MAX_CLIENTS && (clients[i].fd == 0 || strcmp(clients[i].name, msg.name) != 0); i++);
        if (i == MAX_CLIENTS) break;
        del_client(i);
        break;
    }
    case WORK_DONE: {
        int i = client_by_name(msg.name);
        if (i < MAX_CLIENTS) {
            clients[i].inactive = 0;
            clients[i].working--;
        }
        printf("JOB %lu DONE BY %s:\n%s\n", msg.id, (char*) msg.name, (char*) msg.content);
        break;
    }
    case PONG: {
        int i = client_by_name(msg.name);
        if (i < MAX_CLIENTS)
            clients[i].inactive = 0;
    }
    }

    pthread_mutex_unlock(&client_mutex);

    del_msg(msg);
}

void del_client(int i)
{
    clients[i].fd = 0;
    free(clients[i].name);
    clients[i].name = NULL;
    free(clients[i].addr);
    clients[i].addr = NULL;
    clients[i].addr_len = 0;
    clients[i].working = 0;
    clients[i].inactive = 0;
}

int client_by_name(char *name)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == 0) continue;
        if (strcmp(clients[i].name, name) == 0)
            break;
    }
    return i;
}

void send_msg(int i, sock_msg_t msg) {
    ssize_t head_size = sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size) + sizeof(msg.id);
    ssize_t size = head_size + msg.size + 1 + msg.name_size + 1;
    int8_t *buff = malloc(size);
    if (buff == NULL) die_errno();

    memcpy(buff, &msg.type, sizeof(msg.type));
    memcpy(buff + sizeof(msg.type), &msg.size, sizeof(msg.size));
    memcpy(buff + sizeof(msg.type) + sizeof(msg.size), &msg.name_size, sizeof(msg.name_size));
    memcpy(buff + sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size), &msg.id, sizeof(msg.id));

    if (msg.size > 0 && msg.content != NULL)
        memcpy(buff + head_size, msg.content, msg.size + 1);
    if (msg.name_size > 0 && msg.name != NULL)
        memcpy(buff + head_size + msg.size + 1, msg.name, msg.name_size + 1);

    sendto(clients[i].fd, buff, size, 0, clients[i].addr, clients[i].addr_len);

    free(buff);
}

void send_empty(int i, sock_msg_type_t reply)
{
    sock_msg_t msg = { reply, 0, 0, 0, NULL, NULL };
    send_msg(i, msg);
};

sock_msg_t get_msg(int sock, struct sockaddr *addr, socklen_t *addr_len)
{
    sock_msg_t msg;
    ssize_t head_size = sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size) + sizeof(msg.id);
    int8_t buff[head_size];
    if (recv(sock, buff, head_size, MSG_PEEK) < head_size)
        die("Uknown message from client");

    memcpy(&msg.type, buff, sizeof(msg.type));
    memcpy(&msg.size, buff + sizeof(msg.type), sizeof(msg.size));
    memcpy(&msg.name_size, buff + sizeof(msg.type) + sizeof(msg.size), sizeof(msg.name_size));
    memcpy(&msg.id, buff + sizeof(msg.type) + sizeof(msg.size) + sizeof(msg.name_size), sizeof(msg.id));

    ssize_t size = head_size + msg.size + 1 + msg.name_size + 1;
    int8_t* buffer = malloc(size);

    if (recvfrom(sock, buffer, size, 0, addr, addr_len) < size) {
        die("Uknown message from client");
    }

    if (msg.size > 0) {
        msg.content = malloc(msg.size + 1);
        if (msg.content == NULL) die_errno();
        memcpy(msg.content, buffer + head_size, msg.size + 1);
    } else {
        msg.content = NULL;
    }

    if (msg.name_size > 0) {
        msg.name = malloc(msg.name_size + 1);
        if (msg.name == NULL) die_errno();
        memcpy(msg.name, buffer + head_size + msg.size + 1, msg.name_size + 1);
    } else {
        msg.name = NULL;
    }

    free(buffer);

    return msg;
}

void del_msg(sock_msg_t msg)
{
    if (msg.content != NULL)
        free(msg.content);
    if (msg.name != NULL)
        free(msg.name);
}

void int_handler(int signo)
{
    exit(0);
}

void cleanup(void)
{
    close(web_sock);
    close(un_sock);
    unlink(un_path);
    close(epoll);
}
