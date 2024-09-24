.POSIX:
CC = gcc
CFLAGS =  -Wall -Wextra -Wpedantic -O5 -g
LDFLAGS =
LDLIBS =
PREFIX = /usr/local

all: xor

.SUFFIXES: .c .o
.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o xor

.PHONY: all clean
