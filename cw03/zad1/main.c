#define _XOPEN_SOURCE 500
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void tree(const char* path, const char* rel_path);
void show_dir(const char* path, const char* rel_path);
void die_errno();
void die(char*);

char* gop;
time_t gtime;

int main(int argc, char* argv[]) {
	if (argc != 2)
		die("Not enough arguments");

	char* dir_name = realpath(argv[1], NULL);
	if (!dir_name) die_errno();

	tree(dir_name, ".");

	free(dir_name);

	return 0;
}

void tree(const char* path, const char* rel_path) {
	assert(path && rel_path);

	show_dir(path, rel_path);
	DIR* dir;
	struct dirent* ent;
	struct stat stat;

	if (!(dir = opendir(path)))
		die_errno();

	while ((ent = readdir(dir))) {
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;

		char* new_path = malloc(strlen(path) + strlen(ent->d_name) + 2);
		char* new_rel_path = malloc(strlen(rel_path) + strlen(ent->d_name) + 2);
		if (!new_path || !new_rel_path) die("Could not allocate memory");
		sprintf(new_path, "%s/%s", path, ent->d_name);
		if (strcmp(".", rel_path) == 0) {
			sprintf(new_rel_path, "%s", ent->d_name);
		} else {
			sprintf(new_rel_path, "%s/%s", rel_path, ent->d_name);
		}

		if (lstat(new_path, &stat) == -1) die_errno();

		if (S_ISDIR(stat.st_mode)) {
			tree(new_path, new_rel_path);
		}
		free(new_path);
		free(new_rel_path);
	}

	if (closedir(dir) == -1) die_errno();
}

void show_dir(const char* path, const char* rel_path) {
	assert(path);
	pid_t child_pid = fork();

	if (child_pid == -1) {
		die_errno();
	} else if (child_pid == 0) {
		printf("\nDIR:'%s'\nPID: %d\n", rel_path, (int) getpid());
		execlp("ls", "ls", "-l", path, NULL);
	} else {
		int status;
		wait(&status);
		if (status) {
			die("Child process failed");
		}
	}
}

void die_errno() {
	die(strerror(errno));
}

void die(char* msg) {
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
