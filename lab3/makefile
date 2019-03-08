# 360 Lab 3 Makefile
#Samuel Steinberg -- September 2018

CC = gcc

INCLUDES = -I/home/plank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/plank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a
EXECUTABLES: fakemake
all: $(EXECUTABLES)
.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) -c $*.c
fakemake: fakemake.o
		$(CC) $(CFLAGS) -o fakemake fakemake.o $(LIBS)
clean:
		rm core $(EXECUTABLES) *.o
