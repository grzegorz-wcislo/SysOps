#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

int sig_count = 0;

static void kill_catch_usr(int sig, siginfo_t *info, void *context);
static void queue_catch_usr(int sig, siginfo_t *info, void *context);
static void catch_rt(int sig, siginfo_t *info, void *context);

int main(int argc, char* argv[]) {
	if (argc != 2)
		die("Specify mode");

	char *mode = argv[1];

	if (strcmp(mode, "KILL") == 0) {
		init_signals(kill_catch_usr);
	} else if (strcmp(mode, "SIGQUEUE") == 0) {
		init_signals(queue_catch_usr);
	} else if (strcmp(mode, "SIGRT") == 0) {
		init_rt_signals(catch_rt);
	} else {
		die("Unknown mode");
	}

	printf("PID: %d\n", (int) getpid());

	while (1) pause();
	return 0;
}

static void kill_catch_usr(int sig, siginfo_t *info, void *context) {
	if (sig == SIGUSR1) {
		++sig_count;
		if (kill(info->si_pid, SIGUSR1) != 0)
			die_errno();
	} else {
		if (kill(info->si_pid, SIGUSR2) != 0)
			die_errno();
		exit(0);
	}
}

static void queue_catch_usr(int sig, siginfo_t *info, void *context) {
	if (sig == SIGUSR1) {
		++sig_count;
		if (sigqueue(info->si_pid, SIGUSR1, info->si_value) != 0)
			die_errno();
	} else {
		if (kill(info->si_pid, SIGUSR2) != 0)
			die_errno();
		exit(0);
	}
}

static void catch_rt(int sig, siginfo_t *info, void *context) {
	if (sig == SIGRTMIN) {
		++sig_count;
		if (kill(info->si_pid, SIGRTMIN) != 0)
			die_errno();
	} else {
		if (kill(info->si_pid, SIGRTMAX) != 0)
			die_errno();
		exit(0);
	}
}
