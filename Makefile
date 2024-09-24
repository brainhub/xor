.POSIX:
CC = gcc
CFLAGS =  -Wall -Wextra -Wpedantic -O2 -g
LDFLAGS =
LDLIBS =
PREFIX = /usr/local

all: xor

.SUFFIXES: .c .o
.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

sha3test: xor.o 
	$(CC) $(LDFLAGS) -o $@ xor.o $(LDLIBS)

.PHONY: all 
