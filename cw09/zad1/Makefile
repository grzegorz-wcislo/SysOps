CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread -lm

.PHONY: all clean

all: main

main: utils.o utils.h

clean:
	$(RM) main *.o
