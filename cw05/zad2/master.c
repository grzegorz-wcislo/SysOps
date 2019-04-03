#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define	LINE_MAX 256

void die_errno(void);
void die(char*);

int	main(int argc, char *argv[])
{
	if (argc != 2)
		die("Pass the name of the pipe");

	char line[LINE_MAX];

	if (mkfifo(argv[1], S_IWUSR | S_IRUSR) < 0)
		die_errno();

	FILE *pipe = fopen(argv[1], "r");
	if (!pipe) die_errno();

	while (fgets(line, LINE_MAX, pipe) != NULL) {
		if (write(1, line, strlen(line)) < 0)
			die_errno();
	}

	fclose(pipe);
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
