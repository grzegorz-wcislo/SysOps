CC = gcc
CFLAGS = -Wall -ggdb -std=c99 -pthread -lm

.PHONY: all clean test

all: gimp

gimp: utils.o utils.h

test: test.sh gimp
	./test.sh > Times.txt
	$(RM) test-output.pgm

clean:
	$(RM) gimp *.o
