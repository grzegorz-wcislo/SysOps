#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "shared.h"

msg_queue make_server_queue()
{
	char *home = getenv("HOME");
	if (home == NULL) die("$HOME is not set");
	key_t key = ftok(home, PROJ_ID);
	msg_queue q = msgget(key, IPC_CREAT | IPC_EXCL | 0600);
	if (q < 0) die_errno();
	return q;
}

msg_queue get_server_queue()
{
	char *home = getenv("HOME");
	if (home == NULL) die("$HOME is not set");
	key_t key = ftok(home, PROJ_ID);
	msg_queue q = msgget(key, IPC_EXCL);
	if (q < 0) die_errno();
	return q;
}

msg_queue get_private_queue()
{
	char *home = getenv("HOME");
	if (home == NULL) die("$HOME is not set");
	key_t key = ftok(home, (int) getpid());
	msg_queue q = msgget(key, IPC_CREAT | IPC_EXCL | 0600);
	if (q < 0) die_errno();
	return q;
}

msg_queue get_queue_by_key(void *key_ptr)
{
	key_t key;
	memcpy(&key, key_ptr, sizeof(key_t));
	msg_queue q = msgget(key, IPC_EXCL);
	if (q < 0) die_errno();
	return q;
}

void get_queue_key(msg_queue q, void *key_ptr)
{
	struct msqid_ds buf;
	if (msgctl(q, IPC_STAT, &buf) < 0) die_errno();
	memcpy(key_ptr, &buf.msg_perm.__key, sizeof(key_t));
}

void del_queue(msg_queue q)
{
	msgctl(q, IPC_RMID, NULL);
}

msg make_message(char *content, mtype type)
{
	return make_custom_message(content, strlen(content)+1, type);
}

msg make_custom_message(void* content, size_t size, mtype type)
{
	msg msg;
	msg.type = type;
	memcpy(msg.content, content, size);
	msg.sender_id = getpid();
	switch (type) {
	case STOP:
		msg.priority = 1;
		break;
	case LIST:
	case FRIENDS:
		msg.priority = 2;
		break;
	default:
		msg.priority = 3;
	}
	return msg;
}

msg get_message(msg_queue q)
{
	msg msg;
	while (1) {
		if (msgrcv(q, &msg, sizeof(msg) - sizeof(long), -10, 0) < 0) {
			if (errno == EIDRM) break;
			if (errno != EINTR) die_errno();
		} else break;
	}
	return msg;
}

void send_message(msg_queue q, msg msg)
{
	msgsnd(q, &msg, sizeof(msg) - sizeof(long), 0);
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
