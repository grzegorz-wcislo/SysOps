#define _XOPEN_SOURCE 500
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TIME_FMT "_%Y-%m-%d_%H-%M-%S"

typedef struct {
	char *file_name;
	pid_t pid;
	int interval;
	int stopped;
} child;

typedef struct {
	child *list;
	int count;
} children;

// parent
void register_sig_handler(void);
static void sig_parent_handler(int signo);
void list(children *children);
child *find_child(children *children, pid_t child_pid);
void stop_pid(children *children, pid_t child_pid);
void stop(child *child);
void stop_all(children *children);
void start_pid(children *children, pid_t child_pid);
void start(child *child);
void start_all(children *children);
void end(children *children);

// spawning children
pid_t monitor(char *file_name, int interval);
void watch(char *file_name, int interval);

// children
int stopped = 0;
int finish = 0;
static void sig_child_handler(int signo);

// utility
char *read_file(char *file_name);
int lines(char *string);
void show_errno();
void die_errno();
void die(char *msg);

int main(int argc, char* argv[]) {
	register_sig_handler();

	if (argc != 2) die("You should pass file name");
	char* listname = argv[1];
	char *list_buffer = read_file(listname);
	if (!list_buffer) die("Could not read file");
	int expected_child_count = lines(list_buffer);
	printf("%d\n", expected_child_count);
	child *children_list = calloc(expected_child_count, sizeof(child));

	char *file_name = malloc(255);
	if (!file_name) die_errno();
	int interval;

	char *curr_line = list_buffer;
	int child_count = 0;
	while (curr_line) {
		char *next_line = strchr(curr_line, '\n');
		if (next_line) next_line[0] = '\0';

		if (sscanf(curr_line, "%255s %d", file_name, &interval) == 2) {
			pid_t child_pid = monitor(file_name, interval);
			if (child_pid) {
				child *child = &children_list[child_count];
				child->file_name = malloc(strlen(file_name) + 1);
				assert(child->file_name);
				strcpy(child->file_name, file_name);
				child->pid = child_pid;
				child->interval = interval;
				child_count++;
			}
		} else {
			fprintf(stderr, "Ignoring line '%s', incorrect monitor format\n", curr_line);
		}

		if (next_line) next_line[0] = '\n';
		curr_line = next_line && strcmp(next_line, "\n") != 0
			? &next_line[1]
			: NULL;
	}

	children_list = realloc(children_list, child_count * sizeof(child));

	free(list_buffer);
	free(file_name);

	children children;
	children.list = children_list;
	children.count = child_count;

	list(&children);

	char command[15];

	while (!finish) {
		fgets(command, 15, stdin);

		if (strcmp(command, "LIST\n") == 0) {
			list(&children);
		} else if (strcmp(command, "END\n") == 0 || finish) {
			break;
		} else if (strcmp(command, "STOP ALL\n") == 0) {
			stop_all(&children);
		} else if (strncmp(command, "STOP ", 5) == 0) {
			int pid = atoi(command+5);
			if (pid > 0) stop_pid(&children, pid);
			else fprintf(stderr, "Invalid pid '%s'", command+5);
		} else if (strcmp(command, "START ALL\n") == 0) {
			start_all(&children);
		} else if (strncmp(command, "START ", 6) == 0) {
			int pid = atoi(command+6);
			if (pid > 0) start_pid(&children, pid);
			else fprintf(stderr, "Invalid pid '%s'", command+6);
		} else {
			fprintf(stderr, "Command was not recognized\n");
		}
	}

	for (int i = 0; i < child_count; i++) {
		int status;
		kill(children.list[i].pid, SIGUSR2);
		pid_t child_pid = waitpid(children.list[i].pid, &status, 0);
		if (WIFEXITED(status)) {
			printf("Process %d has made %d backups\n", (int) child_pid, WEXITSTATUS(status));
		} else {
			printf("Process %d has been terminated abnormally and the number of backups made is unknown\n", (int) child_pid);
		}
	}

	for (int i = 0; i < child_count; i++) {
		free(children.list[i].file_name);
	}

	free(children.list);

	return 0;
}

void register_sig_handler(void) {
	struct sigaction act;
	act.sa_handler = sig_parent_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL))
		die("Can't catch SIGINT");
}

static void sig_parent_handler(int signo) {
	if (signo == SIGINT)
		finish = 1;
}

void list(children *children) {
	assert(children);

	for (int i = 0; i < children->count; i++) {
		printf("Process %d is monitoring '%s' every %d seconds\n",
			   children->list[i].pid,
			   children->list[i].file_name,
			   children->list[i].interval
			);
	}
}

child *find_child(children *children, pid_t child_pid) {
	assert(children);

	for (int i = 0; i < children->count; i++)
		if (children->list[i].pid == child_pid)
			return &children->list[i];


	fprintf(stderr, "Child with pid %d does not exist\n", child_pid);
	return NULL;
}

void stop_pid(children *children, pid_t child_pid) {
	child *child = find_child(children, child_pid);
	if (child) stop(child);
}

void stop(child *child) {
	assert(child);

	if (child->stopped) {
		fprintf(stderr, "Child with PID %d was alreadry stopped\n", child->pid);
		return;
	}

	if (kill(child->pid, SIGUSR1) == -1) show_errno();
	else {
		printf("Child with PID %d stopped\n", child->pid);
		child->stopped = 1;
	}
}

void stop_all(children *children) {
	assert(children);

	for (int i = 0; i < children->count; i++)
		stop(&children->list[i]);
}

void start_pid(children *children, pid_t child_pid) {
	child *child = find_child(children, child_pid);
	if (child) start(child);
}

void start(child *child) {
	assert(child);

	if (!child->stopped) {
		fprintf(stderr, "Child with PID %d is already running\n", child->pid);
		return;
	}

	if (kill(child->pid, SIGUSR1) == -1) show_errno();
	else {
		printf("Child with PID %d restarted\n", child->pid);
		child->stopped = 0;
	}
}

void start_all(children *children) {
	assert(children);

	for (int i = 0; i < children->count; i++)
		start(&children->list[i]);
}

pid_t monitor(char* file_name, int interval) {
	pid_t child_pid = fork();

	if (child_pid == 0) {
		struct sigaction act;
		act.sa_handler = sig_child_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		if (sigaction(SIGUSR1, &act, NULL) || sigaction(SIGUSR2, &act, NULL))
			die("Can't catch SIGUSR");
		watch(file_name, interval);
	} else if (child_pid == -1) {
		fprintf(stderr, "Failed to start process for: %s\n", file_name);
		return 0;
	}
	return child_pid;
}

void watch(char *file_name, int interval) {
	int n = 0;
	struct stat stat;

	if (lstat(file_name, &stat) == -1) {
		fprintf(stderr, "File '%s' does not exist\n", file_name);
		exit(0);
	}

	time_t last_mod = stat.st_mtime;

	char *contents = read_file(file_name);
	if (!contents) {
		fprintf(stderr, "Could not read file\n");
		exit(n);
	}

	char *file_name_date = malloc(strlen(file_name) + 30);
	strcpy(file_name_date, file_name);

	while (1) {
		sleep(interval);

		if (finish) break;
		if (stopped) continue;

		if (lstat(file_name, &stat) == -1) {
			fprintf(stderr, "File '%s' does not exist\n", file_name);
			break;
		}

		if (stat.st_mtime > last_mod) {
			strftime(&file_name_date[strlen(file_name)], 30, TIME_FMT, gmtime(&last_mod));

			FILE *backup = fopen(file_name_date, "w");

			fwrite(contents, 1, strlen(contents), backup);

			fclose(backup);

			free(contents);
			contents = read_file(file_name);
			if (!contents) {
				fprintf(stderr, "Could not read file\n");
				exit(n);
			}

			last_mod = stat.st_mtime;
			n++;
		}
	}

	free(contents);
	free(file_name_date);

	exit(n);
}

static void sig_child_handler(int signo) {
	if (signo == SIGUSR1) {
		stopped = !stopped;
		printf("%d\n", stopped);
	} else {
		finish = 1;
	}
}

char *read_file(char *file_name) {
	assert(file_name);

	struct stat stat;
	if (lstat(file_name, &stat) != 0) die_errno();

	FILE* file = fopen(file_name, "r");
	if (!file) return NULL;

	char *filebuffer = malloc(stat.st_size + 1);
	if (!filebuffer || fread(filebuffer, 1, stat.st_size, file) != stat.st_size) {
		free(filebuffer);
		fclose(file);
		return NULL;
	}

	filebuffer[stat.st_size] = '\0';

	fclose(file);

	return filebuffer;
}

int lines(char *string) {
	assert(string);

	int count = 0;
	char *i = string;
	while (i) {
		i = strchr(i, '\n');
		if (i) {
			i++;
			count++;
		}
	}
	return count;
}

void show_errno() {
	fputs(strerror(errno), stderr);
}

void die_errno() {
	die(strerror(errno));
}

void die(char* msg) {
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
