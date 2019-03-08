#Lab 4 Makefile
#Samuel Steinberg -- September-October 2018

CC = gcc

INCLUDES = -I/home/plank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/plank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a
EXECUTABLES: jtar
all: $(EXECUTABLES)
.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) -c $*.c
jtar: jtar.o
	$(CC) $(CFLAGS) -o jtar jtar.o $(LIBS)
clean:
	rm core $(EXECUTABLES) *.o
