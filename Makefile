.POSIX:
CC = gcc

# -static will need e.g. `dnf install glibc-static`
CFLAGS =  -Wall -Wextra -Wpedantic -O5 -g -static
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
