CC = gcc
CFLAGS = -Wall -std=c11 -ggdb
ARFLAGS = rcs
LDFLAGS = -L ./

.PHONY: all test clean

all: main_static main_shared main_dynamic

main_static: main.o libgreatfind.a
	$(CC) $(CFLAGS) $< $(LDFLAGS) libgreatfind.a -o $@

main_shared: LDFLAGS += -Wl,-rpath=./
main_shared: main.o libgreatfind.so
	$(CC) $(CFLAGS) $< $(LDFLAGS) -lgreatfind -o $@

main_dynamic: LDFLAGS += -Wl,-rpath=./ -ldl -D DLL
main_dynamic: main.c libgreatfind.so
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

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

test: test.sh main_static main_shared main_dynamic
	./test.sh >> raport3a.txt

clean:
	$(RM) main_static main_shared main_dynamic *.o *.a *.so
