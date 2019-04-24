#ifndef _SHARED_H
#define _SHARED_H

#include <sys/types.h>

#define PROJ_ID 13
#define MAX_CLIENTS 100
#define MAX_CONTENT_LEN 3000
#define MSG_LEN sizeof(msg)

typedef int msg_queue;

typedef enum {
	INIT,
	ECHO,
	LIST,
	FRIENDS,
	TO_ALL,
	TO_FRIENDS,
	TO_ONE,
	STOP,
} mtype;

typedef struct msg {
	long priority;
	mtype type;
	pid_t sender_id;
	char content[MAX_CONTENT_LEN];
} msg;

msg_queue make_server_queue();
msg_queue get_server_queue();
msg_queue get_private_queue();
msg_queue get_queue_by_key(void*);
void get_queue_key(msg_queue, void*);
void del_queue(msg_queue);

msg make_message(char*, mtype);
msg make_custom_message(void*, size_t, mtype);
msg get_message(msg_queue);
void send_message(msg_queue, msg);

void show_errno(void);
void die_errno(void);
void die(char*);

#endif
