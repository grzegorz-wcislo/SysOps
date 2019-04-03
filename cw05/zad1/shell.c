#define _POSIX_C_SOURCE 1

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMDS 20
#define MAX_ARGS 20

typedef struct {
	char *name;
	char **argv;
} cmd;

typedef struct {
	cmd *list;
	size_t size;
} cmds;

char *read_file(char*);
cmds parse_commands(char*);
cmd parse_command(char*);
void create_pipes(cmds);
void spawn_children(cmds);

void show_errno(void);
void die_errno(void);
void die(char*);

int main(int argc, char *argv[])
{
	if (argc != 2)
		die("Pass a file name");

	char *command_string = read_file(argv[1]);
	cmds commands = parse_commands(command_string);
	spawn_children(commands);

	for (int i = 0; i < commands.size; i++)
		wait(NULL);

	free(command_string);

	return 0;
}

char *read_file(char *file_name)
{
	FILE* file = fopen(file_name, "r");
	if (!file) die_errno();

	fseek(file, 0 , SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0 , SEEK_SET);

	char *file_buffer = malloc(file_size + 1);
	if (!file_buffer) die_errno();

	size_t chars_read = fread(file_buffer, 1, file_size, file);

	if (chars_read != file_size)
		die("Error reading file");

	if (fclose(file) != 0)
		die_errno();

	return file_buffer;
}

cmds parse_commands(char *commands)
{
	assert(commands);
	char *saveptr;
	cmd *list = calloc(MAX_CMDS + 1, sizeof(cmd));
	if (!list) die_errno();

	char *cmd_str = strtok_r(commands, "|\n", &saveptr);

	if (cmd_str == NULL)
		die("The commands should not be empty");

	int i;
	for (i = 0; cmd_str != NULL; i++, cmd_str = strtok_r(NULL, "|\n", &saveptr)) {
		if (i < MAX_CMDS)
			list[i] = parse_command(cmd_str);
		else
			die("Too many commands");
	}

	cmds cmds = {};
	cmds.list = list;
	cmds.size = i;

	return cmds;
}

cmd parse_command(char* cmd_str)
{
	assert(cmd_str);
	char *saveptr;
	cmd cmd = {};
	char **args = calloc(MAX_ARGS + 1, sizeof(char*));
	if (!args) die_errno();

	char *arg = strtok_r(cmd_str, " ", &saveptr);

	if (arg == NULL)
		die("The command should not be empty");

	for (int i = 0; arg != NULL; i++, arg = strtok_r(NULL, " ", &saveptr)) {
		if (i == 0)
			cmd.name = arg;
		if (i < MAX_ARGS)
			args[i] = arg;
		else
			die("Too many arguments in command");
	}

	cmd.argv = args;

	return cmd;
}

void spawn_children(cmds commands)
{
	int pipes[2][2];

	int i = 0;
	for (i = 0; i < commands.size; i++) {
		cmd cmd = commands.list[i];

		if (i > 0) {
            close(pipes[i % 2][0]);
            close(pipes[i % 2][1]);
        }

		if(pipe(pipes[i % 2]) == -1) die_errno();
		pid_t pid = fork();
		if (pid == 0) {
			if (i != commands.size - 1) {
                close(pipes[i % 2][0]);
                if (dup2(pipes[i % 2][1], STDOUT_FILENO) < 0)
					die_errno();
            }
            if (i != 0) {
                close(pipes[(i+1) % 2][1]);
                if (dup2(pipes[(i + 1) % 2][0], STDIN_FILENO) < 0)
					die_errno();
            }
			execvp(cmd.name, cmd.argv);
		}
	}
	close(pipes[i % 2][0]);
	close(pipes[i % 2][1]);
}

void show_errno(void)
{
	fputs(strerror(errno), stderr);
}

void die_errno(void)
{
	die(strerror(errno));
}

void die(char* msg)
{
	assert(msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
