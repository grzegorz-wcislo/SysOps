#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "shared.h"

msg_queue client_queue = -1;
msg_queue server_queue = -1;

void send_init();

void *handle_messages();

void handle_command();
void parse_command();
void execute_command(char*);
void read_file(char*);

void handle_init(msg*);
void handle_echo(msg*);
void handle_list();
void handle_stop();

void register_handlers();
void exit_handler();
void int_handler(int);

int main(void)
{
	server_queue = get_server_queue();
	client_queue = get_private_queue();

	register_handlers();

	send_init();

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, handle_messages, NULL);

	while (1) handle_command();

	return 0;
}

void send_init()
{
	msg msg = make_message("", INIT);
	get_queue_key(client_queue, msg.content);
	send_message(server_queue, msg);
}

void *handle_messages()
{
	while (1) {
		msg msg = get_message(client_queue);
		switch (msg.type) {
		case INIT:
			puts("handling INIT");
			handle_init(&msg);
			break;
		case ECHO:
			puts("handling ECHO");
			handle_echo(&msg);
			break;
		case STOP:
			puts("handling STOP");
			handle_stop();
			break;
		default:;
		}
	}
}

void handle_command()
{
	static char cmd_buffer[256];
	if (fgets(cmd_buffer, 256, stdin) == NULL) die_errno();
	cmd_buffer[strcspn(cmd_buffer, "\n")] = 0;

	execute_command(cmd_buffer);
}

void execute_command(char* cmd)
{
	if (strncmp("ECHO ", cmd, 5) == 0) {
		puts("sending ECHO");
		msg msg = make_message(cmd+5, ECHO);
		send_message(server_queue, msg);
	} else if (strcmp("LIST", cmd) == 0) {
		puts("sending LIST");
		msg msg = make_message("", LIST);
		send_message(server_queue, msg);
	} else if (strncmp("FRIENDS ", cmd, 8) == 0) {
		puts("sending FRIENDS");
		msg msg = make_message(cmd+8, FRIENDS);
		send_message(server_queue, msg);
	} else if (strncmp("2ALL ", cmd, 5) == 0) {
		puts("sending 2ALL");
		msg msg = make_message(cmd+5, TO_ALL);
		send_message(server_queue, msg);
	} else if (strncmp("2FRIENDS ", cmd, 9) == 0) {
		puts("sending 2FRIENDS");
		msg msg = make_message(cmd+9, TO_FRIENDS);
		send_message(server_queue, msg);
	} else if (strncmp("2ONE ", cmd, 5) == 0) {
		puts("sending 2ONE");
		msg msg = make_message(cmd+5, TO_ONE);
		send_message(server_queue, msg);
	} else if (strncmp("READ ", cmd, 5) == 0) {
		puts("reading commands from file");
		read_file(cmd+5);
	} else if (strcmp("STOP", cmd) == 0) {
		exit(0);
	}
}

void read_file(char *file_name)
{
	static char cmd_buffer[256];

	FILE *file = fopen(file_name, "r");
	if (file == NULL) {
		show_errno();
		return;
	}

	while(1) {
		if (fgets(cmd_buffer, 256, file) == NULL) break;
		cmd_buffer[strcspn(cmd_buffer, "\n")] = 0;

		execute_command(cmd_buffer);
	}

	fclose(file);
}

void handle_init(msg *msg)
{
	if (strcmp("OK", msg->content) != 0)
		die("Server is too busy");
}

void handle_echo(msg* msg)
{
	puts(msg->content);
}

void handle_stop()
{
	exit(0);
}

void register_handlers()
{
	struct sigaction sa;
	sa.sa_handler = int_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) die_errno();

	atexit(exit_handler);
}

void exit_handler()
{
	if (client_queue != -1) del_queue(client_queue);
	if (server_queue != -1) {
		msg msg = make_message("", STOP);
		send_message(server_queue, msg);
	}
}

void int_handler(int signum)
{
	exit(0);
}
