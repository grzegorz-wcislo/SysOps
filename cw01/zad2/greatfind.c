#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "greatfind.h"

blocktable* create_table(int size) {
	if (size <= 0) {
		fprintf(stderr,"Table size has to be positive\n");
		exit(1);
	}

	blocktable* bt = calloc(1, sizeof(blocktable));

	char** table = calloc(size, sizeof(char*));

	if (bt == NULL || table == NULL) {
		fprintf(stderr,"Out of memory, cannot allocate new table\n");
		exit(1);
	}

	bt->table = table;
	bt->size = size;

	return bt;
}

void find(blocktable* bt) {
	if (bt == NULL || bt->table == NULL) {
		fprintf(stderr,"Given block table does not exist\n");
		exit(1);
	} if (bt->dir == NULL || bt->file == NULL || bt->temp == NULL) {
		fprintf(stderr,"dir name, file name and tempfile name has to be specified\n");
		exit(1);
	}

	char* command = calloc(1,strlen(bt->dir) + strlen(bt->file) + strlen(bt->temp) + 40);
	sprintf(command, "find \"%s\" -iname \"%s\" > \"%s\" 2>/dev/null", bt->dir, bt->file, bt->temp);

	system(command);
	free(command);
}

int allocate_tempfile_block(blocktable* bt) {
	if (bt == NULL || bt->table == NULL) {
		fprintf(stderr,"Given block table does not exist\n");
		exit(1);
	}

	int free_i;
	for(free_i = 0; free_i < bt->size && bt->table[free_i] != NULL; free_i++);

	if (bt->table[free_i] != NULL) {
		fprintf(stderr,"Table is full\n");
		exit(1);
	} if (bt->temp == NULL) {
		fprintf(stderr,"Temp file was not defined\n");
		exit(1);
	}

	FILE* tmp = fopen(bt->temp, "r\n");
	if (tmp == NULL) {
		fprintf(stderr,"Temp file could not be located\n");
		exit(1);
	}

	fseek(tmp, 0 ,SEEK_END);
	long size = ftell(tmp);
	rewind(tmp);

	if ((bt->table[free_i] = calloc(1,size+1)) == NULL) {
		fprintf(stderr,"Out of memory, could not allocate new block\n");
		fclose(tmp);
		exit(1);
	}

	fread(bt->table[free_i], size, 1, tmp);
	if (ferror(tmp)) {
		fprintf(stderr,"Could not read the temp file\n");
		fclose(tmp);
		exit(1);
	}

	fclose(tmp);

	return free_i;
}

void delete_block(blocktable* bt, int index) {
	if (bt == NULL || bt->table == NULL) {
		fprintf(stderr,"Given block table does not exist\n");
		exit(1);
	} if (index < 0 || index >= bt->size) {
		fprintf(stderr,"Given block is out of range\n");
		exit(1);
	} if (bt->table[index] == NULL) {
		fprintf(stderr,"Given block was already deleted\n");
		exit(1);
	}
	free(bt->table[index]);
	bt->table[index] = NULL;
}

void delete_table(blocktable* bt) {
	if (bt != NULL) {
		if (bt->table != NULL) {
			for (int i = 0; i < bt->size; i++) {
				if (bt->table[i] != NULL)
					free(bt->table[i]);
			}
			free(bt->table);
		}
		free(bt);
	}
}
