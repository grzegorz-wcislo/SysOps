#define _XOPEN_SOURCE 500
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define TIME_FMT "%d.%m.%Y %H:%M:%S"

void tree(char* path, char* op, time_t time);
static int tree_nftw(const char *path, const struct stat *stat, int typeflag, struct FTW *ftwbuf);
void show_file(const char* path, const struct stat* stat);
time_t parse_time(char* s);
int time_valid(time_t file_time, char* op, time_t time);
void die_errno();
void die(char*);

char* gop;
time_t gtime;

int main(int argc, char* argv[]) {
	if (argc < 5)
		die("Not enough arguments");

	char* dir_name = realpath(argv[1], NULL);
	if (!dir_name) die_errno();

	char* op = argv[2];
	time_t time = parse_time(argv[3]);

	char* mode = argv[4];

	if (strcmp(mode, "1") == 0) {
		tree(dir_name, op, time);
	} else if (strcmp(mode, "2") == 0) {
		gop = op;
		gtime = time;
		nftw(dir_name, tree_nftw, 20, FTW_PHYS);
	} else {
		die("Unknown mode, please specify 1 or 2");
	}

	free(dir_name);

	return 0;
}

void tree(char* path, char* op, time_t time) {
	assert(path && op);

	DIR* dir;
	struct dirent* ent;
	struct stat stat;

	if (!(dir = opendir(path)))
		die_errno();

	while ((ent = readdir(dir))) {
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;

		char* new_path = malloc(strlen(path) + strlen(ent->d_name) + 2);
		if (!new_path) die("Could not allocate memory");
		sprintf(new_path, "%s/%s", path, ent->d_name);

		if (lstat(new_path, &stat) == -1) die_errno();

		if (time_valid(stat.st_mtime, op, time))
			show_file(new_path, &stat);

		if (S_ISDIR(stat.st_mode)) {
			tree(new_path, op, time);
		}
		free(new_path);
	}

	if (closedir(dir) == -1) die_errno();
}

static int tree_nftw(const char *path, const struct stat *stat, int typeflag, struct FTW *ftwbuf) {
	if (ftwbuf->level == 0) return 0;
	if (!time_valid(stat->st_mtime, gop, gtime)) return 0;

	show_file(path, stat);

	return 0;
}

void show_file(const char* path, const struct stat* stat) {
	assert(path && stat);

	printf("%s\t", path);

	if (S_ISREG(stat->st_mode))
		printf("zwykły plik\t");
	else if (S_ISDIR(stat->st_mode))
		printf("katalog\t");
	else if (S_ISCHR(stat->st_mode))
		printf("urządzenie znakowe\t");
	else if (S_ISBLK(stat->st_mode))
		printf("urzączenie blokowe\t");
	else if (S_ISFIFO(stat->st_mode))
		printf("potok nazwany\t");
	else if (S_ISLNK(stat->st_mode))
		printf("link symboliczny\t");
	else
		printf("soket\t");

	printf("%ld\t", stat->st_size);

	char* buffer = malloc(30);
	if (!buffer) die("Could not allocate memory");
	strftime(buffer, 30, TIME_FMT, localtime(&stat->st_atime));
	printf("%s\t", buffer);
	strftime(buffer, 30, TIME_FMT, localtime(&stat->st_mtime));
	printf("%s\n", buffer);
	free(buffer);
}

time_t parse_time(char* s) {
	assert(s);

	struct tm date;
	char* res = strptime(s, TIME_FMT, &date);
    if (res == NULL || *res != '\0') {
		die("Incorrect date format");
	}

	time_t time = mktime(&date);
	if (time == -1) die_errno();
	return time;
}

int time_valid(time_t file_time, char* op, time_t time) {
	if(strcmp(op, "=") == 0) {
		return file_time == time;
	} else if (strcmp(op, ">") == 0) {
		return file_time > time;
	} else if (strcmp(op, "<") == 0) {
		return file_time < time;
	} else {
		die("Unknown date operand");
	}
	return 0;
}
void die_errno() {
	die(strerror(errno));
}

void die(char* msg) {
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
