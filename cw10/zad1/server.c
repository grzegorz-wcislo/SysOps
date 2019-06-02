#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

void handle_reg(int);
void handle_call(int);

void send_msg(int sock, sock_msg_t msg);
void send_empty(int, sock_msg_type_t);

void del_client(int);
void del_sock(int);

int client_by_name(char*);

sock_msg_t get_msg(int);
void del_msg(sock_msg_t);

void int_handler(int);
void cleanup(void);

int main(int argc, char *argv[]) {
    if (argc != 3) die("Pass: tcp port, unix socket path");

    init_server(argv[1], argv[2]);

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) == -1) die_errno();

        if (event.data.fd < 0)
            handle_reg(-event.data.fd);
        else
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

    if ((web_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die_errno();

    if (bind(web_sock, (const struct sockaddr *) &web_addr, sizeof(web_addr)))
        die_errno();

    if (listen(web_sock, 64) == -1)
        die_errno();

    // init unix socket
    struct sockaddr_un un_addr;
    un_addr.sun_family = AF_UNIX;

    snprintf(un_addr.sun_path, UNIX_PATH_MAX, "%s", un_path);

    if ((un_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        die_errno();

    if (bind(un_sock, (const struct sockaddr *) &un_addr, sizeof(un_addr)))
        die_errno();

    if (listen(un_sock, MAX_CLIENTS) == -1)
        die_errno();

    // init epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) == -1)
        die_errno();

    event.data.fd = -web_sock;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, web_sock, &event) == -1)
        die_errno();

    event.data.fd = -un_sock;
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
            send_msg(clients[min_i].fd, msg);
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
                send_empty(clients[i].fd, PING);
            }
        }
        pthread_mutex_unlock(&client_mutex);
        sleep(10);
    }
}

void handle_reg(int sock)
{
    puts("Client registered");
    int client = accept(sock, NULL, NULL);
    if (client == -1) die_errno();
struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    event.data.fd = client;

    if (epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event) == -1)
        die_errno();
}

void handle_call(int sock)
{
    sock_msg_t msg = get_msg(sock);
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
            del_sock(sock);
            break;
        }

        clients[i].fd = sock;
        clients[i].name = malloc(msg.size + 1);
        if (clients[i].name == NULL) die_errno();
        strcpy(clients[i].name, msg.name);
        clients[i].working = 0;
        clients[i].inactive = 0;

        send_empty(sock, OK);
        break;
    }
    case UNREGISTER: {
        int i;
        for (i = 0; i < MAX_CLIENTS && strcmp(clients[i].name, msg.name) != 0; i++);
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
    del_sock(clients[i].fd);
    clients[i].fd = 0;
    clients[i].name = NULL;
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

void del_sock(int sock) {
    epoll_ctl(epoll, EPOLL_CTL_DEL, sock, NULL);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

void send_msg(int sock, sock_msg_t msg) {
    write(sock, &msg.type, sizeof(msg.type));
    write(sock, &msg.size, sizeof(msg.size));
    write(sock, &msg.name_size, sizeof(msg.name_size));
    write(sock, &msg.id, sizeof(msg.id));
    if (msg.size > 0) write(sock, msg.content, msg.size);
    if (msg.name_size > 0) write(sock, msg.name, msg.name_size);
}

void send_empty(int sock, sock_msg_type_t reply)
{
    sock_msg_t msg = { reply, 0, 0, 0, NULL, NULL };
    send_msg(sock, msg);
};

sock_msg_t get_msg(int sock)
{
    sock_msg_t msg;
    if (read(sock, &msg.type, sizeof(msg.type)) != sizeof(msg.type))
        die("Uknown message from client");
    if (read(sock, &msg.size, sizeof(msg.size)) != sizeof(msg.size))
        die("Uknown message from client");
    if (read(sock, &msg.name_size, sizeof(msg.name_size)) != sizeof(msg.name_size))
        die("Uknown message from client");
    if (read(sock, &msg.id, sizeof(msg.id)) != sizeof(msg.id))
        die("Uknown message from client");
    if (msg.size > 0) {
        msg.content = malloc(msg.size + 1);
        if (msg.content == NULL) die_errno();
        if (read(sock, msg.content, msg.size) != msg.size) {
            die("Uknown message from client");
        }
    } else {
        msg.content = NULL;
    }
    if (msg.name_size > 0) {
        msg.name = malloc(msg.name_size + 1);
        if (msg.name == NULL) die_errno();
        if (read(sock, msg.name, msg.name_size) != msg.name_size) {
            die("Uknown message from client");
        }
    } else {
        msg.name = NULL;
    }
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
