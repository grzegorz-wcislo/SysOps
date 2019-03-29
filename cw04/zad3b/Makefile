CC = gcc
CFLAGS = -Wall -ggdb

TARGETS = sender catcher

.PHONY: all clean

all: $(TARGETS)

$(TARGETS): helper.o

clean:
	$(RM) $(TARGETS) *.o
