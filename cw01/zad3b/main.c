#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#ifndef DLL
#include "greatfind.h"
#endif

#ifdef DLL
#include <dlfcn.h>

void * greatfind;

typedef struct {
	char** table;
	int size;
	char* dir;
	char* file;
	char* temp;
} blocktable;

blocktable* (*create_table)(int);
void (*find)(blocktable*);
int (*allocate_tempfile_block)(blocktable*);
void (*delete_block)(blocktable*, int);
void (*delete_table)(blocktable*);

void dynamic_load_fail() {
	fprintf(stderr, "Could not load dynamic library");
	exit(1);
}
#endif

typedef struct {
	clock_t real_start;
	struct tms start;
	double elapsed_real;
	double elapsed_user;
	double elapsed_sys;
	int used;
} timer;

void show_help_and_exit();
void time_reset(timer*, int);
void time_start(timer*);
void time_stop(timer*);
void time_show(timer*, char*);
double calc_time();
int parse_int(char*);

int main(int argc, char *argv[]) {
	#ifdef DLL
	greatfind = dlopen("libgreatfind.so", RTLD_LAZY);
	if (greatfind == NULL) dynamic_load_fail();
	create_table = dlsym(greatfind, "create_table");
	delete_table = dlsym(greatfind, "delete_table");
	if (create_table == NULL || delete_table == NULL) dynamic_load_fail();
	#endif

	timer timers[4];
	time_reset(timers, 4);
	timer* find_timer  = &timers[0];
	timer* alloc_timer = &timers[1];
	timer* del_timer   = &timers[2];
	timer* alt_timer   = &timers[3];

	if (argc < 2)
		show_help_and_exit();

	int size = parse_int(argv[1]);

	blocktable* bt = create_table(size);

	int arg = 2;

	while (arg < argc) {
		if (strncmp(argv[arg], "s", 1) == 0) {
			if (++arg >= argc) break;
			bt->dir = argv[arg];
			if (++arg >= argc) break;
			bt->file = argv[arg];
			if (++arg >= argc) break;
			bt->temp = argv[arg];

			#ifdef DLL
			if (find == NULL) {
				find = dlsym(greatfind, "find");
				allocate_tempfile_block = dlsym(greatfind, "allocate_tempfile_block");
				if (find == NULL || allocate_tempfile_block == NULL) dynamic_load_fail();
			}
			#endif

			time_start(find_timer);
			find(bt);
			time_stop(find_timer);

			time_start(alloc_timer);
			allocate_tempfile_block(bt);
			time_stop(alloc_timer);

			arg++;
		} else if (strncmp(argv[arg], "d", 1) == 0) {
			if (++arg >= argc) break;
			int index = parse_int(argv[arg]);
			if (index < 0 || index >= bt->size || bt->table[index] == NULL) {
				printf("Cannot delete from: '%s'\n", argv[arg]);
				break;
			}

			#ifdef DLL
			if (delete_block == NULL) {
				delete_block = dlsym(greatfind, "delete_block");
				if (delete_block == NULL) dynamic_load_fail();
			}
			#endif

			time_start(del_timer);
			delete_block(bt, index);
			time_stop(del_timer);

			arg++;
		} else if (strncmp(argv[arg], "a", 1) == 0) {
			if (++arg >= argc) break;
			int count = parse_int(argv[arg]);
			if (++arg >= argc) break;
			bt->temp = argv[arg];

			time_start(alt_timer);
			#ifdef DLL
			if (delete_block == NULL) {
				delete_block = dlsym(greatfind, "delete_block");
				allocate_tempfile_block = dlsym(greatfind, "allocate_tempfile_block");
				if (delete_block == NULL || allocate_tempfile_block == NULL) dynamic_load_fail();
			}
			#endif

			for(int i = 0; i < count; i++) {
				int index = allocate_tempfile_block(bt);
				delete_block(bt, index);
			}

			time_stop(alt_timer);

			arg++;
		} else {
			printf("Unknown option: '%s'\n", argv[arg]);
		    break;
		}
	}

	time_show(find_timer, "Finding");
	time_show(alloc_timer, "Allocating");
	time_show(del_timer, "Deleting");
	time_show(alt_timer, "Alternating allocating and deleting");

	delete_table(bt);

	#ifdef DLL
	dlclose(greatfind);
	#endif
}

void show_help_and_exit() {
	printf("Usage:\t<table_size> [command]...\n" \
		"Command can be one of:\n" \
		"\ts <dir> <file> <tempfile>\t- search for <file> in <dir>\n" \
		"\td <index>\t- delete result from <index>\n" \
		"\ta <count> <file>\t- read and delete <file> block <count> times\n");
	exit(2);
}

int parse_int(char* input) {
	return atoi(input);
}

void time_reset(timer* timers, int count) {
	for(int i = 0; i < count; i++) {
		timers[i].elapsed_real = 0;
		timers[i].elapsed_user = 0;
		timers[i].elapsed_sys = 0;
		timers[i].used = 0;
	}
}

void time_start(timer* t) {
	t->real_start = times(&(t->start));
	t->used = 1;
}

void time_stop(timer* t) {
	struct tms end;
	clock_t real_end = times(&end);

	t->elapsed_real += calc_time(t->real_start, real_end);
	t->elapsed_user += calc_time(t->start.tms_utime, end.tms_utime);
	t->elapsed_sys  += calc_time(t->start.tms_stime, end.tms_stime);
}

void time_show(timer* t, char* task) {
	if (t->used) {
		printf(task);
		putchar('\n');
		printf("  real    %0.2fs\n", t->elapsed_real);
		printf("  user    %0.2fs\n", t->elapsed_user);
		printf("  sys     %0.2fs\n", t->elapsed_sys);
	}
}

double calc_time(clock_t t1, clock_t t2) {
	return (double) (t2-t1) / sysconf(_SC_CLK_TCK);
}
