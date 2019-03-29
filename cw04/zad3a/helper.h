#ifndef HELPER_H
#define HELPER_H

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

void init_signals(void(*) (int, siginfo_t*, void*));
void init_rt_signals(void(*) (int, siginfo_t*, void*));

void show_errno(void);
void die_errno(void);
void die(char *msg);

#endif
