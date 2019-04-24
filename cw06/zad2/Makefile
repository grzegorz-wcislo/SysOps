CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread
LDFLAGS = -lrt

.PHONY: all clean

all: client server

client server: shared.o

shared.o: shared.c shared.h

clean:
	$(RM) client server *.o

