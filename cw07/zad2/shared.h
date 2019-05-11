#ifndef _SHARED_H
#define _SHARED_H

#include <sys/types.h>
#include <sys/time.h>

#define PROJ_ID 13

int parse_pos_int(char*);

struct timeval curr_time();
long int time_diff(struct timeval, struct timeval);
void print_time(struct timeval);
void print_curr_time(void);

void show_errno(void);
void die_errno(void);
void die(char* msg);

#endif
