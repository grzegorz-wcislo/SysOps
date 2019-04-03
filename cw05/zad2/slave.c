#define	_DEFAULT_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define	LINE_MAX 256

void die_errno(void);
void die(char*);

int	main(int argc, char* argv[]) {
	srand(time(NULL));
	if (argc != 3)
		exit(EXIT_FAILURE);

	char date_res[LINE_MAX];
	char pipe_string[LINE_MAX+20];

	int count = atoi(argv[2]);

	printf("%d\n", (int) getpid());

	int pipe = open(argv[1], O_WRONLY);
	if	(pipe < 0) die_errno();

	for (int i = 0; i < count; i++) {
		FILE *date = popen("date", "r");
		fgets(date_res, LINE_MAX, date);

	if (sprintf(pipe_string, "%d @ %s", (int) getpid(), date_res) < 0)
			die("Could	not write to string");

		if (write(pipe, pipe_string, strlen(pipe_string)) < 0)
			die_errno();

		sleep(rand() % 4 + 2);
	}

	close(pipe);
	return 0;
}

void die_errno(void)
{
	die(strerror(errno));
}

void die(char* msg)
{
	assert(msg);
	fprintf(stderr,	"%s\n", msg);
	exit(1);
}
