CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -ansi

.PHONY: all clean test
.SILENT: test test_dir 

all: main

test: main test_dir
	./main test_dir

test_dir:
	mkdir test_dir
	mkdir test_dir/foo
	touch test_dir/foo/file1
	touch test_dir/file2
	ln -s /etc/passwd test_dir/passwd
	ln -s /usr/lib test_dir/lib

clean:
	$(RM) main
	$(RM) -r test_dir
