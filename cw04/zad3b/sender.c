#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

int expected_sig_count = 0;
int sig_count = 0;

static void kill_catch_usr(int sig, siginfo_t *info, void *context);
static void queue_catch_usr(int sig, siginfo_t *info, void *context);
static void catch_rt(int sig, siginfo_t *info, void *context);

int main(int argc, char* argv[]) {
	if (argc != 4)
		die("Specify PID, number of signals and mode");

	int pid = atoi(argv[1]);
	expected_sig_count = atoi(argv[2]);
	char *mode = argv[3];

	if (strcmp(mode, "KILL") == 0) {
		init_signals(kill_catch_usr);
		if (kill(pid, SIGUSR1) != 0)
			die_errno();
	} else if (strcmp(mode, "SIGQUEUE") == 0) {
		init_signals(queue_catch_usr);
		union sigval val = {0};
		if (sigqueue(pid, SIGUSR1, val) != 0)
			die_errno();
	} else if (strcmp(mode, "SIGRT") == 0) {
		init_rt_signals(catch_rt);
		if (kill(pid, SIGRTMIN) != 0)
			die_errno();
	} else {
		die("Unknown mode");
	}

	while (1) pause();
	return 0;
}

static void kill_catch_usr(int sig, siginfo_t *info, void *context) {
	if (sig == SIGUSR1) {
		++sig_count;
		if (sig_count < expected_sig_count) {
			if (kill(info->si_pid, SIGUSR1) != 0)
				die_errno();
		} else {
			if (kill(info->si_pid, SIGUSR2) != 0)
				die_errno();
		}
	} else {
		printf("Expected: %d, Actual: %d\n", expected_sig_count, sig_count);
		exit(0);
	}
}

static void queue_catch_usr(int sig, siginfo_t *info, void *context) {
	if (sig == SIGUSR1) {
		++sig_count;
		printf("%dth SIGUSR1 caught\n", info->si_value.sival_int);
		if (sig_count < expected_sig_count) {
			union sigval val = {sig_count};
			if (sigqueue(info->si_pid, SIGUSR1, val) != 0)
				die_errno();
		} else {
			if (kill(info->si_pid, SIGUSR2) != 0)
				die_errno();
		}
	} else {
		printf("Expected: %d, Actual: %d\n", expected_sig_count, sig_count);
		exit(0);
	}
}

static void catch_rt(int sig, siginfo_t *info, void *context) {
	if (sig == SIGRTMIN) {
		++sig_count;
		if (sig_count < expected_sig_count) {
			if (kill(info->si_pid, SIGRTMIN) != 0)
				die_errno();
		} else {
			if (kill(info->si_pid, SIGRTMAX) != 0)
				die_errno();
		}
	} else {
		printf("Expected: %d, Actual: %d\n", expected_sig_count, sig_count);
		exit(0);
	}
}
