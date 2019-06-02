CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread -lm

.PHONY: all clean

all: client server

client server: utils.o utils.h

clean:
	$(RM) client server *.o
