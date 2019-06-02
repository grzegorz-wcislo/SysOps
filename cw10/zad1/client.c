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

int sock;
char* name;

void init_client(char*, char*, char*);

sock_msg_t get_msg(void);
void del_msg(sock_msg_t);

void send_msg(sock_msg_t*);
void send_empty(sock_msg_type_t);
void send_done(int, char*);

void int_handler(int);
void cleanup(void);

int main(int argc, char *argv[]) {
    if (argc != 4) die("Pass: client name, variant (WEB/UNIX), address");

    init_client(argv[1], argv[2], argv[3]);

    while (1) {
        sock_msg_t msg = get_msg();

        switch (msg.type) {
        case OK: {
            break;
        }
        case PING: {
            send_empty(PONG);
            break;
        }
        case NAME_TAKEN: die("Name is already taken");
        case FULL: die("Server is full");
        case WORK: {
            puts("Doing work...");
            char *buffer = malloc(100 + 2 * msg.size);
            if (buffer == NULL) die_errno();
            sprintf(buffer, "echo '%s' | awk '{for(x=1;$x;++x)print $x}' | sort | uniq -c", (char*) msg.content);
            FILE *result = popen(buffer, "r");
            if (result == 0) { free(buffer); break; }
            int n = fread(buffer, 1, 99 + 2 * msg.size, result);
            buffer[n] = '\0';
            puts("Work done...");
            send_done(msg.id, buffer);
            free(buffer);
            break;
        }
        default: break;
        }

        del_msg(msg);
    }
}

void init_client(char* n, char* variant, char* address)
{
    // register atexit
    if (atexit(cleanup) == -1) die_errno();

    // register int handler
    if (signal(SIGINT, int_handler) == SIG_ERR)
        show_errno();

    // set name
    name = n;

    // parse address
    if (strcmp("WEB", variant) == 0) {
        strtok(address, ":");
        char *port = strtok(NULL, ":");
        if (port == NULL) die("Specify a port");

        uint32_t in_addr = inet_addr(address);
        if (in_addr == INADDR_NONE) die("Invalid address");

        uint16_t port_num = (uint16_t) parse_pos_int(port);
        if (port_num < 1024)
            die("Invalid port number");

        struct sockaddr_in web_addr;
        memset(&web_addr, 0, sizeof(struct sockaddr_in));

        web_addr.sin_family = AF_INET;
        web_addr.sin_addr.s_addr = in_addr;
        web_addr.sin_port = htons(port_num);

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            die_errno();

        if (connect(sock, (const struct sockaddr *) &web_addr, sizeof(web_addr)) == -1)
            die_errno();
    } else if (strcmp("UNIX", variant) == 0) {
        char* un_path = address;

        if (strlen(un_path) < 1 || strlen(un_path) > UNIX_PATH_MAX)
            die("Invalid unix socket path");

        struct sockaddr_un un_addr;
        un_addr.sun_family = AF_UNIX;
        snprintf(un_addr.sun_path, UNIX_PATH_MAX, "%s", un_path);

        if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
            die_errno();

        if (connect(sock, (const struct sockaddr *) &un_addr, sizeof(un_addr)) == -1)
            die_errno();
    } else {
        die("Unknown variant");
    }

    send_empty(REGISTER);
}

void send_msg(sock_msg_t* msg)
{
    write(sock, &msg->type, sizeof(msg->type));
    write(sock, &msg->size, sizeof(msg->size));
    write(sock, &msg->name_size, sizeof(msg->name_size));
    write(sock, &msg->id, sizeof(msg->id));
    if (msg->size > 0) write(sock, msg->content, msg->size);
    if (msg->name_size > 0) write(sock, msg->name, msg->name_size);
}

void send_empty(sock_msg_type_t type)
{
    sock_msg_t msg = { type, 0, strlen(name) + 1, 0, NULL, name };
    send_msg(&msg);
};

void send_done(int id, char *content)
{
    sock_msg_t msg = { WORK_DONE, strlen(content) + 1, strlen(name) + 1, id, content, name };
    send_msg(&msg);
}

sock_msg_t get_msg(void)
{
    sock_msg_t msg;
    if (read(sock, &msg.type, sizeof(msg.type)) != sizeof(msg.type))
        die("Uknown message from server");
    if (read(sock, &msg.size, sizeof(msg.size)) != sizeof(msg.size))
        die("Uknown message from server");
    if (read(sock, &msg.name_size, sizeof(msg.name_size)) != sizeof(msg.name_size))
        die("Uknown message from server");
    if (read(sock, &msg.id, sizeof(msg.id)) != sizeof(msg.id))
        die("Uknown message from server");
    if (msg.size > 0) {
        msg.content = malloc(msg.size + 1);
        if (msg.content == NULL) die_errno();
        if (read(sock, msg.content, msg.size) != msg.size) {
            die("Uknown message from server");
        }
    } else {
        msg.content = NULL;
    }
    if (msg.name_size > 0) {
        msg.name = malloc(msg.name_size + 1);
        if (msg.name == NULL) die_errno();
        if (read(sock, msg.name, msg.name_size) != msg.name_size) {
            die("Uknown message from server");
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
    send_empty(UNREGISTER);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}
