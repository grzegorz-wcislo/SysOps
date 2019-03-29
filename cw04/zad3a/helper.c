#include "helper.h"

void init_signals(void(*fun) (int, siginfo_t*, void*)) {
	sigset_t signals;

	if (sigfillset(&signals) == -1)
		die_errno();

	sigdelset(&signals, SIGUSR1);
	sigdelset(&signals, SIGUSR2);

	if (sigprocmask(SIG_BLOCK, &signals, NULL) != 0)
		die_errno();

	if (sigemptyset(&signals) == -1)
		die_errno();

	struct sigaction usr_action;
	usr_action.sa_sigaction = fun;
	usr_action.sa_mask = signals;
	usr_action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGUSR1, &usr_action, NULL) == -1
		|| sigaction(SIGUSR2, &usr_action, NULL) == -1)
		die_errno();
}

void init_rt_signals(void(*fun) (int, siginfo_t*, void*)) {
	sigset_t signals;

	if (sigfillset(&signals) == -1)
		die_errno();

	sigdelset(&signals, SIGRTMIN);
	sigdelset(&signals, SIGRTMAX);

	if (sigprocmask(SIG_BLOCK, &signals, NULL) != 0)
		die_errno();

	if (sigemptyset(&signals) == -1)
		die_errno();

	struct sigaction usr_action;
	usr_action.sa_sigaction = fun;
	usr_action.sa_mask = signals;
	usr_action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGRTMIN, &usr_action, NULL) == -1
		|| sigaction(SIGRTMAX, &usr_action, NULL) == -1)
		die_errno();
}

void show_errno(void) {
	fputs(strerror(errno), stderr);
}

void die_errno(void) {
	die(strerror(errno));
}

void die(char* msg) {
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
