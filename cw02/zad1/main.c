#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <unistd.h>

#define advance() if (++arg >= argc) die_parse(cmd);

void generate(char*, int, int);
void sort_lib(char* filename, int count, int len);
void sort_sys(char* filename, int count, int len);
void copy_lib(char* src_name, char* tgt_name, int count, int len);
void copy_sys(char* src_name, char* tgt_name, int count, int len);
void die(char*);
void die_errno();
void die_parse(char*);
double calc_time(clock_t t1, clock_t t2);

int main(int argc, char* argv[]) {
	if (argc < 2)
		exit(1);

	int arg = 1;

	while (arg < argc) {
		struct tms start;
		times(&start);

		char* cmd;
		if (strcmp(argv[arg], cmd = "generate") == 0) {
			advance();
			char* filename = argv[arg];
			advance();
			int count = atoi(argv[arg]);
			advance();
			int len = atoi(argv[arg]);
			generate(filename, count, len);
		}
		else if (strcmp(argv[arg], cmd = "sort") == 0) {
			advance();
			char* filename = argv[arg];
			advance();
			int count = atoi(argv[arg]);
			advance();
			int len = atoi(argv[arg]);
			advance();
			char* type = argv[arg];
			if (strcmp(type, "sys") == 0) {
				sort_sys(filename, count, len);
			} else if (strcmp(type, "lib") == 0) {
				sort_lib(filename, count, len);
			} else die("Unknown option");
		}
		else if (strcmp(argv[arg], cmd = "copy") == 0) {
			advance();
			char* filename1 = argv[arg];
			advance();
			char* filename2 = argv[arg];
			advance();
			int count = atoi(argv[arg]);
			advance();
			int len = atoi(argv[arg]);
			advance();
			char* type = argv[arg];
			if (strcmp(type, "sys") == 0) {
				copy_sys(filename1, filename2, count, len);
			} else if (strcmp(type, "lib") == 0) {
				copy_lib(filename1, filename2, count, len);
			} else die("Unknown option");
		}
		else {
			die("Unknown command");
		}
		++arg;

		struct tms end;
		times(&end);

		printf("  user    %0.2fs\n", calc_time(start.tms_utime, end.tms_utime));
		printf("  sys     %0.2fs\n", calc_time(start.tms_stime, end.tms_stime));
	}

	return 0;
}

void generate(char* filename, int count, int len) {
	FILE* file = fopen(filename, "w");
	if (!file) die_errno();

	FILE* dev_null = fopen("/dev/urandom", "r");
	if (!dev_null) die_errno();

	for (int i = 0; i < count; i++)
		for (int j = 0; j < len; j++)
			fputc(fgetc(dev_null), file);

	if (fclose(dev_null) != 0) die_errno();

	if (fclose(file)) die_errno();
}

void sort_lib(char* filename, int count, int len) {
	FILE* file = fopen(filename, "r+");
	if (!file) die_errno();

	unsigned char* buf1 = malloc(len * sizeof(char));
	unsigned char* buf2 = malloc(len * sizeof(char));
	if (!buf1 || !buf2) die("Could not allocate memory for buffers");

	for (int i = 0; i < count - 1; i++) {
		fseek(file, i * len, SEEK_SET);
		if(fread(buf1, sizeof(char), len, file) != len)
			die("Error reading file");

		unsigned char minimum = buf1[0];
		int min_j = i;

		for (int j = i+1; j < count; j++) {
			fseek(file, j * len, SEEK_SET);
			unsigned char next = (unsigned char) fgetc(file);
			if (next < minimum) {
				minimum = next;
				min_j = j;
			}
		}

		fseek(file, min_j * len, SEEK_SET);

		if (fread(buf2, sizeof(char),  len, file) != len)
			die("Error reading file");

		fseek(file, min_j * len, SEEK_SET);

		fwrite(buf1, sizeof(char), len, file);
		if (ferror(file))
			die("Error writing file");

		fseek(file, i * len, SEEK_SET);

		fwrite(buf2, sizeof(char), len, file);
		if (ferror(file))
			die("Error writing file");
	}

	free(buf1);
	free(buf2);

	if (fclose(file) != 0) die_errno();
}

void sort_sys(char* filename, int count, int len) {
	int file = open(filename, O_RDWR);
	if (!file) die_errno();

	unsigned char* buf1 = malloc(len * sizeof(char));
	unsigned char* buf2 = malloc(len * sizeof(char));
	if (!buf1 || !buf2) die("Could not allocate memory for buffers");

	for (int i = 0; i < count - 1; i++) {
		lseek(file, i * len, SEEK_SET);
		if (read(file, buf1, len * sizeof(char)) != len)
			die("Error reading file");

		unsigned char minimum = buf1[0];
		int min_j = i;

		for (int j = i+1; j < count; j++) {
			lseek(file, j * len, SEEK_SET);
			unsigned char next;
			if (read(file, &next, sizeof(char)) != 1)
				die("Error reading file");
			if (next < minimum) {
				minimum = next;
				min_j = j;
			}
		}

		lseek(file, min_j * len, SEEK_SET);

		if (read(file, buf2, len * sizeof(char)) != len)
			die("Error reading file");

		lseek(file, min_j * len, SEEK_SET);

		if (write(file, buf1, len * sizeof(char)) == -1) die_errno();

		lseek(file, i * len, SEEK_SET);

		if (write(file, buf2, len * sizeof(char)) == -1) die_errno();
	}

	free(buf1);
	free(buf2);

	if (close(file) != 0) die_errno();
}

void copy_lib(char* src_name, char* tgt_name, int count, int len) {
	FILE* source = fopen(src_name, "r");
	if (!source) die_errno();
	FILE* target = fopen(tgt_name, "w");
	if (!target) die_errno();

	unsigned char* buffer = malloc(len);
	if (!buffer) die("Could not allocate memory for buffer");

	for (int i = 0; i < count; i++) {
		if (fread(buffer, sizeof(char), len, source) != len)
			die_errno();
		fwrite(buffer, sizeof(char), len, target);
		if (ferror(target))
			die("Error writing file");
	}

	free(buffer);

	if (fclose(source) != 0) die_errno();
	if (fclose(target) != 0) die_errno();
}

void copy_sys(char* src_name, char* tgt_name, int count, int len) {
	int source = open(src_name, O_RDONLY);
	if (!source) die_errno();
	int target = open(tgt_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (!target) die_errno();

	unsigned char* buffer = malloc(len);
	if (!buffer) die("Could not allocate memory for buffer");

	for (int i = 0; i < count; i++) {
		if (read(source, buffer, len) != len)
			die_errno();
		if (write(target, buffer, len) == -1) die_errno();
	}

	free(buffer);

	if (close(source) != 0) die_errno();
	if (close(target) != 0) die_errno();
}

void die(char* msg) {
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	exit(1);
}

void die_errno() {
	die(strerror(errno));
}

void die_parse(char* cmd) {
	fprintf(stderr, "Could not parse command: %s\n", cmd);
	exit(1);
}

double calc_time(clock_t t1, clock_t t2) {
	return (double) (t2-t1) / sysconf(_SC_CLK_TCK);
}
