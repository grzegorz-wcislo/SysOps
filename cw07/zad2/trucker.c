#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 1

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "shared.h"
#include "belt.h"

void register_handlers();
void exit_handler();
void int_handler(int);

belt *b;

int main(int argc, char *args[])
{
    if (argc != 4) die("Pass X, K and M");
    int x = parse_pos_int(args[1]);
    int k = parse_pos_int(args[2]);
    int m = parse_pos_int(args[3]);

    b = create_belt(k, m);

    register_handlers();

    truck t = { x, x };
    while (1) {
        get_package(b, &t);
    }

    return 0;
}

void register_handlers()
{
	struct sigaction sa;
	sa.sa_handler = int_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) die_errno();

	atexit(exit_handler);
}

void exit_handler()
{
    if (b) delete_belt(b);
}

void int_handler(int _signum)
{
    b->trucker_done = 1;
}
