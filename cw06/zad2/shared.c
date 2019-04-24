#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <mqueue.h>

#include "shared.h"

msg_queue make_server_queue()
{
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MESSAGE_QUEUE_SIZE;
    attr.mq_msgsize = sizeof(msg);

    msg_queue q = mq_open("/server", O_RDONLY | O_CREAT | O_EXCL, 0666, &attr);

	if (q < 0) die_errno();
	return q;
}

msg_queue get_server_queue()
{
    msg_queue q = mq_open("/server", O_WRONLY);

	if (q < 0) die_errno();
	return q;
}

msg_queue get_private_queue()
{
	char *buff = malloc(40);
	if (!buff) die_errno();
	sprintf(buff, "/client_%d", getpid());

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MESSAGE_QUEUE_SIZE;
    attr.mq_msgsize = sizeof(msg);

    msg_queue q = mq_open(buff, O_RDONLY | O_CREAT | O_EXCL, 0666, &attr);

	if (q < 0) die_errno();
	free(buff);
	return q;
}

msg_queue get_queue_by_key(void *key_ptr)
{
	msg_queue q = mq_open(key_ptr, O_WRONLY);
	if (q < 0) die_errno();
	mq_unlink(key_ptr);
	return q;
}

void get_queue_key(msg_queue q, void *key_ptr)
{
	char *buff = malloc(40);
	if (!buff) die_errno();
	sprintf(buff, "/client_%d", getpid());

	strcpy(key_ptr, buff);
	free(buff);
}

void del_queue(msg_queue q)
{
	mq_close(q);
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
		msg.priority = 3;
		break;
	case LIST:
	case FRIENDS:
		msg.priority = 2;
		break;
	default:
		msg.priority = 1;
	}
	return msg;
}

msg get_message(msg_queue q)
{
	msg msg;
	while (1) {
		if (mq_receive(q, (char*) &msg, sizeof(msg), NULL) < 0) {
			if (errno != EINTR) die_errno();
		} else break;
	}
	return msg;
}

void send_message(msg_queue q, msg msg)
{
	mq_send(q, (char*) &msg, sizeof(msg), msg.priority);
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
