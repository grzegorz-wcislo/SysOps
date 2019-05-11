#ifndef _BELT_H
#define _BELT_H

#include <sys/types.h>
#include <sys/time.h>

typedef struct package {
    int mass;
    pid_t loader;
    struct timeval loaded_at;
} package;

typedef struct belt {
    int first;
    int last;
    int size;
    int max_weight;
    int weight;
    int semaphores;
    int trucker_done;
    int emptied;
    package buffer[];
} belt;

typedef struct truck {
    int capacity;
    int max_capacity;
} truck;

enum sem { BUFF = 0, EMPTY = 1, FILL = 2 };

belt *get_belt(void);
belt *create_belt(int size, int max_weight);
void delete_belt(belt*);

package create_package(int mass);
void timestamp_package(package*);

void put_package(belt*, package);
void get_package(belt*, truck*);

void inc_sem(belt*, int);
void dec_sem(belt*, int);
int dec_sem_nonblock(belt*, int);

#endif
