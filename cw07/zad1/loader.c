#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 1

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "shared.h"
#include "belt.h"

void register_handlers();
void exit_handler();
void int_handler(int);

int main(int argc, char* args[])
{
    if (argc < 2) die("Pass in the number of loaders, their package sizes and then optional lifetimes");

    int child_count = parse_pos_int(args[1]);

    for (int i = 0; i < child_count; i++) {
        if (fork() == 0) {
            int m = parse_pos_int(args[2 + i]);

            belt *b = get_belt();

            register_handlers();

            package pkg = create_package(m);

            if (argc > 2 + i + child_count) {
                for (int j = 0; j < parse_pos_int(args[2 + i + child_count]); j++)
                    put_package(b, pkg);
            } else {
                while (1) {
                    put_package(b, pkg);
                }
            }
        }
    }

    for (int i = 0; i < child_count; i++) {
        wait(NULL);
    }

    return 0;
}

void register_handlers()
{
	struct sigaction sa;
	sa.sa_handler = int_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) die_errno();
}

void int_handler(int _signum)
{
}
