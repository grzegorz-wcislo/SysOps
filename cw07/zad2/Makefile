CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread -lrt

.PHONY: all clean

all: loader trucker

loader trucker: belt.o shared.o

clean:
	$(RM) loader trucker *.o
