CC = gcc
CFLAGS = -Wall -std=c11 -ggdb
ARFLAGS = rcs
LDFLAGS = -L ./

EXECS = main_static main_shared main_dynamic
OPTS = 0 1 s

TARGETS = $(foreach OPT, $(OPTS), $(foreach EXEC, $(EXECS), $(EXEC)_$(OPT)))

.PHONY: all test clean

all: $(TARGETS)

main_static_%: main_%.o libgreatfind.a
	$(CC) $(CFLAGS) -O$* $< $(LDFLAGS) libgreatfind.a -o $@

main_shared_%: LDFLAGS += -Wl,-rpath=./
main_shared_%: main_%.o libgreatfind.so
	$(CC) $(CFLAGS) -O$* $< $(LDFLAGS) -lgreatfind -o $@

main_dynamic_%: LDFLAGS += -Wl,-rpath=./ -ldl -D DLL
main_dynamic_%: main.c libgreatfind.so
	$(CC) $(CFLAGS) -O$* $< $(LDFLAGS) -o $@

main_%.o: main.c
	$(CC) $(CFLAGS) -O$* -c $? -o $@

libgreatfind.a: greatfind_static.o
	$(AR) $(ARFLAGS) $@ $?

libgreatfind.so: CFLAGS += -shared
libgreatfind.so: greatfind_shared.o
	$(CC) $(CFLAGS) $? -o $@

greatfind_shared.o: CFLAGS += -fPIC
greatfind_static.o greatfind_shared.o: greatfind.c
	$(CC) $(CFLAGS) -c $? -o $@

test: test.sh $(TARGETS)
	./test.sh >> raport3b.txt

clean:
	$(RM) main_* *.o *.a *.so
