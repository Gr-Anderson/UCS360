#Lab 7 Makefile
#Samuel Steinberg -- October-November 2018

CC = gcc

INCLUDES = -I/home/plank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/plank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a
EXECUTABLES: jsh
all: $(EXECUTABLES)
.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) -c $*.c
jsh: jsh.o
		$(CC) $(CFLAGS) -o jsh jsh.o $(LIBS)
clean:
		rm core $(EXECUTABLES) *.o
