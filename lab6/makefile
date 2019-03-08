#Lab 5 Makefile
#Samuel Steinberg -- October 2018

CC = gcc

INCLUDES = -I/home/plank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/plank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a
EXECUTABLES: test_malloc
all: $(EXECUTABLES)
.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) -c $*.c
test_malloc: test_malloc.o malloc.o 
	$(CC) $(CFLAGS) -o test_malloc test_malloc.o malloc.o $(LIBS)
test_malloc.o: test_malloc.c
	$(CC) $(CFLAGS) -c test_malloc.c
malloc.o: malloc.c malloc.h
	$(CC) $(CFLAGS) -c malloc.c
clean:
	rm core $(EXECUTABLES) *.o
