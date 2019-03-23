#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define TIME_FMT "%Y.%m.%d %H:%M:%S"

void die_errno();
void die(char*);

int main(int argc, char *argv[]) {
	srand(time(NULL));

	if (argc != 5) die("Params are: file name, pmin, pmax, bytes");

	char *file_name = argv[1];
	int pmin = atoi(argv[2]);
	int pmax = atoi(argv[3]);
	int bytes = atoi(argv[4]);

	char *string = malloc(bytes+1);
	assert(string);
	for (int i = 0; i < bytes; i++) string[i] = 'a';
	string[bytes] = '\0';

	char *date = malloc(30);
	assert(date);

	while (1) {
		int wait = rand() % (abs(pmax - pmin) + 1) + pmin;
		sleep(wait);

		FILE *file = fopen(file_name, "a");

		time_t t = time(NULL);

		strftime(date, 30, TIME_FMT, gmtime(&t));

		fprintf(file, "%d %s %s\n", wait, date, string);

		fclose(file);
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
