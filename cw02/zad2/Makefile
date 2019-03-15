CC = gcc
CFLAGS = -Wall -ggdb

.PHONY: all clean test
.SILENT: test test_dir 

all: main

test: main test_dir
	echo " == opendir, readdir and stat == "
	./main test_dir ">" "01.03.2017 21:55:05" 1
	echo ""

	echo " == nftw == "
	./main test_dir ">" "01.03.2017 21:55:05" 2
	echo ""

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
