#define _XOPEN_SOURCE 500
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TIME_FMT "%Y.%m.%d %H:%M:%S"

int show_date = 1;

static void sig_tstp(int);
static void sig_int(int);

void die(char*);

int main(void) {
	char timebuffer[256];

	struct sigaction sig_tstp_action;
	sig_tstp_action.sa_handler = sig_tstp;
	sigemptyset(&sig_tstp_action.sa_mask);
	sig_tstp_action.sa_flags = 0;

	if (sigaction(SIGTSTP, &sig_tstp_action, NULL))
		die("can't catch SIGTSTP");

	if (signal(SIGINT, sig_int) == SIG_ERR)
		die("can't catch SIGINT");

	while (1) {
		if (show_date) {
			time_t rawtime;
			struct tm *timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(timebuffer, 255, TIME_FMT, timeinfo);
			puts(timebuffer);
		}
		sleep(1);
	}
	return 0;
}

static void sig_tstp(int _) {
	if (show_date) {
		puts("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakończenie programu");
		show_date = 0;
	} else {
		show_date = 1;
	}
}

static void sig_int(int _) {
	puts("Odebrano sygnał SIGINT");

	if (signal(SIGINT, sig_int) == SIG_ERR)
		die("can't catch SIGINT");
	exit(1);
}

void die(char* msg) {
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
