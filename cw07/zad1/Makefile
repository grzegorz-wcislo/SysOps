CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread

.PHONY: all clean

all: loader trucker

loader trucker: belt.o shared.o

clean:
	$(RM) loader trucker *.o
