CC = gcc
CFLAGS = -Wall -ggdb

.PHONY: all clean test

all: main

test: main test.sh
	./test.sh >> wyniki.txt

clean:
	$(RM) main
