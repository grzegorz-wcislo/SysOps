#ifndef _UTILS_H
#define _UTILS_H

#include <sys/time.h>
#include <sys/types.h>

int parse_pos_int(char*);

struct timeval curr_time();
long int time_diff(struct timeval, struct timeval);

void print_time(struct timeval t);
void print_curr_time(void);

void show_errno(void);
void die_errno(void);
void die(char* msg);

#endif
