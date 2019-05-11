#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "shared.h"

int parse_pos_int(char* s)
{
    int i = atoi(s);
    if (i <= 0) {
        fprintf(stderr, "%s should be a positive integer\n", s);
        exit(-1);
    } else {
        return i;
    }
}

struct timeval curr_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t;
}

long int time_diff(struct timeval t1, struct timeval t2)
{
    return (t2.tv_usec - t1.tv_usec + 1000000) % 1000000;
}


void print_time(struct timeval t)
{
    printf("%ld%ld", t.tv_sec, t.tv_usec);
}

void print_curr_time(void)
{
    print_time(curr_time());
}

void show_errno(void)
{
	fputs(strerror(errno), stderr);
}

void die_errno(void)
{
	die(strerror(errno));
}

void die(char* msg)
{
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
