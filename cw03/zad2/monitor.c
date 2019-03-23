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

char *read_file(char *file_name);

int monitor(char* file_name, int interval, int time, int mode);
void watch_1(char* file_name, int interval, int time);
void watch_2(char* file_name, int interval, int time);

void die_errno();
void die(char*);

int main(int argc, char* argv[]) {
	if (argc != 4) die("You should pass file name, time and mode");

	char* listname = argv[1];
	int time = atoi(argv[2]);
	char* mode = argv[3];

	int m = atoi(mode);
	if (m != 1 && m != 2) die("Unknown mode, choose 1 or 2");

	char *list_buffer = read_file(listname);

	char *file_name = malloc(255);
	if (!file_name) die_errno();
	int interval;

	char *curr_line = list_buffer;
	int child_count = 0;
	while (curr_line) {
		char *next_line = strchr(curr_line, '\n');
		if (next_line) next_line[0] = '\0';

		if (sscanf(curr_line, "%255s %d", file_name, &interval) == 2) {
			child_count += monitor(file_name, interval, time, m);
		} else {
			fprintf(stderr, "Ignoring line '%s', incorrect monitor format\n", curr_line);
		}

		if (next_line) next_line[0] = '\n';
		curr_line = next_line && strcmp(next_line, "\n") != 0
			? &next_line[1]
			: NULL;
	}
	free(list_buffer);
	free(file_name);

	for (int i = 0; i < child_count; i++) {
		int status;
		pid_t child_pid = wait(&status);
		if (WIFEXITED(status)) {
			printf("Process %d has made %d backups\n", (int) child_pid, WEXITSTATUS(status));
		} else {
			printf("Process %d has been terminated abnormally and the number of backups made is unknown\n", (int) child_pid);
		}
	}

	return 0;
}

char *read_file(char *file_name) {
	struct stat stat;
	if (lstat(file_name, &stat) != 0) die_errno();

	FILE* file = fopen(file_name, "r");
	if (!file) die_errno();

	char *filebuffer = malloc(stat.st_size + 1);
	if (fread(filebuffer, 1, stat.st_size, file) != stat.st_size)
		die("Could not read file");

	filebuffer[stat.st_size] = '\0';

	if (fclose(file)) die_errno();

	return filebuffer;
}

int monitor(char* file_name, int interval, int time, int mode) {
	pid_t child_pid = fork();

	if (child_pid == 0) {
		if (mode == 1)
			watch_1(file_name, interval, time);
		else
			watch_2(file_name, interval, time);
	} else if (child_pid == -1) {
		fprintf(stderr, "Failed to start process for: %s\n", file_name);
		return 0;
	}
	return 1;
}

void watch_1(char *file_name, int interval, int time) {
	int elapsed = 0;
	int n = 0;
	struct stat stat;

	if (lstat(file_name, &stat) == -1) {
		fprintf(stderr, "File '%s' does not exist\n", file_name);
		exit(0);
	}

	time_t last_mod = stat.st_mtime;

	char *contents = read_file(file_name);
	char *file_name_date = malloc(strlen(file_name) + 30);
	strcpy(file_name_date, file_name);

	while ((elapsed += interval) <= time) {
		sleep(interval);

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
			if (!contents) die_errno();

			last_mod = stat.st_mtime;
			n++;
		}
	}

	free(contents);
	free(file_name_date);

	exit(n);
}

void watch_2(char* file_name, int interval, int time) {
	int elapsed = 0;
	int n = 0;
	struct stat stat;

	if (lstat(file_name, &stat) == -1) {
		fprintf(stderr, "File '%s' does not exist\n", file_name);
		exit(0);
	}

	time_t last_mod = stat.st_mtime;

	char *file_name_date = malloc(strlen(file_name) + 30);

	strcpy(file_name_date, file_name);

	while ((elapsed += interval) <= time) {
		if (lstat(file_name, &stat) == -1) {
			fprintf(stderr, "File '%s' does not exist\n", file_name);
			break;
		}

		strftime(&file_name_date[strlen(file_name)], 30, TIME_FMT, gmtime(&last_mod));

		if (last_mod < stat.st_mtime || n == 0) {
			last_mod = stat.st_mtime;
			pid_t child_pid = vfork();
			if (child_pid == 0) {
				execlp("cp", "cp", file_name, file_name_date, NULL);
			} else if (child_pid == -1) {
				fprintf(stderr, "Could not backup '%s'\n", file_name);
			} else {
				int status;
				wait(&status);
				if (status != 0)
					fprintf(stderr, "Could not backup '%s'\n", file_name);
				else
					n++;
			}
		}
		sleep(interval);
	}
	free(file_name_date);

	exit(n);
}

void die_errno() {
	die(strerror(errno));
}

void die(char* msg) {
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
