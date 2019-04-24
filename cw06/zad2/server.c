#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <mqueue.h>

#include "shared.h"

msg_queue server_queue;

typedef struct {
	pid_t id;
	msg_queue q;
	pid_t friend_ids[MAX_CLIENTS];
} client;

client *clients[MAX_CLIENTS];
int stop = 0;

void get_and_handle_message();

int find_client(pid_t);

void handle_init(msg*);
void handle_echo(msg*);
void handle_list();
void handle_friends(msg*);
void handle_2all(msg*);
void handle_2friends(msg*);
void handle_2one(msg*);
void handle_stop(msg*);

void add_timestamp(msg*);
void add_sender(msg*);

void register_handlers();
void exit_handler();
void int_handler(int);

int main(void)
{
    server_queue = make_server_queue();
	register_handlers();
	while(1) get_and_handle_message();
}

void get_and_handle_message()
{
	msg msg = get_message(server_queue);
	switch (msg.type) {
	case INIT:
		puts("handling INIT");
		handle_init(&msg);
		break;
	case ECHO:
		puts("handling ECHO");
		handle_echo(&msg);
		break;
	case LIST:
		puts("handling LIST");
		handle_list();
		break;
	case FRIENDS:
		puts("handling FRIENDS");
		handle_friends(&msg);
		break;
	case TO_ALL:
		puts("handling 2ALL");
		handle_2all(&msg);
		break;
	case TO_FRIENDS:
		puts("handling 2FRIENDS");
		handle_2friends(&msg);
		break;
	case TO_ONE:
		puts("handling 2ONE");
		handle_2one(&msg);
		break;
	case STOP:
		puts("handling STOP");
		handle_stop(&msg);
		break;
	default:;
	}
}

int find_client(pid_t id) {
	int i = 0;
	for (; i < MAX_CLIENTS && (!clients[i] || clients[i]->id != id); i++);
	return i;
}

void handle_init(msg *msg) {
	msg_queue q = get_queue_by_key(msg->content);
	struct msg reply;

	int i = 0;
	for (; i < MAX_CLIENTS && clients[i]; i++);

	if (i < MAX_CLIENTS) {
		clients[i] = malloc(sizeof(client));
		clients[i]->id = msg->sender_id;
		clients[i]->q = q;
		reply = make_message("OK", INIT);
	} else {
		reply = make_message("ERROR", INIT);
	}

	send_message(q, reply);
}

void handle_echo(msg *msg)
{
	int i = find_client(msg->sender_id);
	if (i < MAX_CLIENTS) {
		add_timestamp(msg);
		struct msg reply = make_message(msg->content, ECHO);
		send_message(clients[i]->q, reply);
	}
}

void handle_list()
{
	printf("Active Clients: ");
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i] == NULL) continue;
		printf("%d ", clients[i]->id);
	}
	printf("\n");
}

void handle_friends(msg *msg)
{
	int sender_i = find_client(msg->sender_id);
	if (sender_i < MAX_CLIENTS) {
		client *sender = clients[sender_i];

		char *token = strtok(msg->content, " ");
		int i = 0;
		while (token != NULL && i < MAX_CLIENTS) {
			pid_t friend_id = atoi(token);
			int j = find_client(friend_id);
			if (j < MAX_CLIENTS) {
				sender->friend_ids[i] = friend_id;
				i++;
			}
			token = strtok(NULL, " ");
		}
		for (; i < MAX_CLIENTS; i++) {
			sender->friend_ids[i] = 0;
		}
	}
}

void handle_2all(msg *msg)
{
	add_sender(msg);
	add_timestamp(msg);
	struct msg reply = make_message(msg->content, ECHO);

	for (int i = 0; i < MAX_CLIENTS; i++)
		if (clients[i] && clients[i]->id != msg->sender_id)
			send_message(clients[i]->q, reply);

}

void handle_2friends(msg *msg)
{
	int sender_i = find_client(msg->sender_id);
	if (sender_i < MAX_CLIENTS) {
		add_sender(msg);
		add_timestamp(msg);
		client *sender = clients[sender_i];
		struct msg reply = make_message(msg->content, ECHO);

		for (int j = 0; j < MAX_CLIENTS; j++)
			if (sender->friend_ids[j] != 0) {
				int friend_i = find_client(sender->friend_ids[j]);
				if (friend_i < MAX_CLIENTS) {
					send_message(clients[friend_i]->q, reply);
				}
			}
	}
}

void handle_2one(msg *msg)
{
	add_sender(msg);
	add_timestamp(msg);
	char *one = strtok(msg->content, " ");
	int i = find_client(atoi(one));
	if (i < MAX_CLIENTS) {
		struct msg reply = make_message(msg->content + strlen(one) + 1, ECHO);
		send_message(clients[i]->q, reply);
	}
}

void handle_stop(msg *msg)
{
	int i = find_client(msg->sender_id);
	if (i < MAX_CLIENTS) {
		del_queue(clients[i]->q);
		free(clients[i]);
		clients[i] = NULL;
		if (stop) {
			for (i = 0; i < MAX_CLIENTS && clients[i] == NULL; i++);
			if (i == MAX_CLIENTS) exit(0);
		}
	}
}

void add_timestamp(msg *msg)
{
	int len = strlen(msg->content);
	time_t t;
	time(&t);
	struct tm *timeinfo;
	timeinfo = localtime(&t);
	strftime(msg->content + len,
			 MAX_CONTENT_LEN - len - 1,
			 " @ %Y-%d-%m %H:%M:%S",
			 timeinfo);
}

void add_sender(msg *msg)
{
	int len = strlen(msg->content);
	sprintf(msg->content + len, " FROM %d", msg->sender_id);
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
	del_queue(server_queue);
}

void int_handler(int signum)
{
	stop = 1;
	mq_unlink("/server");
	int i;
	for (i = 0; i < MAX_CLIENTS && clients[i] == NULL; i++);
	if (i == MAX_CLIENTS)
		exit(0);
	else
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (clients[i] == NULL) continue;
			msg msg = make_message("", STOP);
			send_message(clients[i]->q, msg);
		}
}
