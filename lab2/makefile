#CS 360 Lab 2 Makefile
#Samuel Steinberg -- September 2018

CC = gcc

INCLUDES = -I/home/plank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/plank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a
EXECUTABLES: l2p1 l2p2 l2p3
all: $(EXECUTABLES)
.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c
l2p1: l2p1.o
	$(CC) $(CFLAGS) -o l2p1 l2p1.o $(LIBS)
l2p2: l2p2.o
	$(CC) $(CFLAGS) -o l2p2 l2p2.o $(LIBS)
l2p3: l2p3.o
	$(CC) $(CFLAGS) -o l2p3 l2p3.o $(LIBS)
clean:
	rm core $(EXECUTABLES) *.o
