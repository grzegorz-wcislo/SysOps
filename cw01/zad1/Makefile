CC = gcc
CFLAGS = -Wall -std=c11 -ggdb
ARFLAGS = rcs

.PHOHY: all static shared clean

all: libgreatfind.a libgreatfind.so

static: libgreatfind.a

shared: libgreatfind.so

greatfind_shared.o: CFLAGS += -fPIC
greatfind_static.o greatfind_shared.o: greatfind.c
	$(CC) $(CFLAGS) -c $? -o $@

libgreatfind.a: greatfind_static.o
	$(AR) $(ARFLAGS) $@ $?

libgreatfind.so: CFLAGS += -shared
libgreatfind.so: greatfind_shared.o
	$(CC) $(CFLAGS) -o $@ $?

clean:
	$(RM) *.o *.a *.so
