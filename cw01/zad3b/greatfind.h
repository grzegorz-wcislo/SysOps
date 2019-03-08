#ifndef GREATFIND_H
#define GREATFIND_H

typedef struct {
	char** table;
	int size;
	char* dir;
	char* file;
	char* temp;
} blocktable;

extern blocktable* create_table(int size);
extern void find(blocktable* bt);
extern int allocate_tempfile_block(blocktable* bt);
extern void delete_block(blocktable* bt, int index);
extern void delete_table(blocktable* bt);

#endif
