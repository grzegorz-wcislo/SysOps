CC = gcc
CFLAGS = -Wall -std=c11 -ggdb
LDFLAGS = -L ./
ARFLAGS = rcs

.PHONY: all test clean

all: main_static main_shared

main_static: main.o libgreatfind.a
	$(CC) $(CFLAGS) $< $(LDFLAGS) libgreatfind.a -o $@

main_shared: LDFLAGS += -Wl,-rpath=./
main_shared: main.o libgreatfind.so
	$(CC) $(CFLAGS) $< $(LDFLAGS) -lgreatfind -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $?

libgreatfind.a: greatfind_static.o
	$(AR) $(ARFLAGS) $@ $?

libgreatfind.so: CFLAGS += -shared
libgreatfind.so: greatfind_shared.o
	$(CC) $(CFLAGS) $? -o $@

greatfind_shared.o: CFLAGS += -fPIC
greatfind_static.o greatfind_shared.o: greatfind.c
	$(CC) $(CFLAGS) -c $? -o $@

test: test.sh main_static main_shared
	./test.sh >> raport2.txt

clean:
	$(RM) main_static main_shared *.o *.a *.so
