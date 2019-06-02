#ifndef _UTILS_H
#define _UTILS_H

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>

#define UNIX_PATH_MAX 108
#define MAX_CLIENTS 10

#define debug(format, ...) { printf(format, ##__VA_ARGS__); putchar('\n'); }

typedef enum sock_msg_type_t {
    REGISTER,
    UNREGISTER,
    PING,
    PONG,
    OK,
    NAME_TAKEN,
    FULL,
    FAIL,
    WORK,
    WORK_DONE,
} sock_msg_type_t;

typedef struct sock_msg_t {
    uint8_t type;
    uint64_t size;
    uint64_t name_size;
    uint64_t id;
    void *content;
    char *name;
} sock_msg_t;

typedef struct client_t {
    int fd;
    char *name;
    uint8_t working;
    uint8_t inactive;
} client_t;

int parse_pos_int(char*);

struct timeval curr_time();
long int time_diff(struct timeval, struct timeval);

void print_time(struct timeval t);
void print_curr_time(void);

void show_errno(void);
void die_errno(void);
void die(char* msg);

#endif
