#Lab 8 Makefile
#Samuel Steinberg -- November 2018

CC = gcc

INCLUDES = -I/home/plank/cs360/include
CFLAGS = -g $(INCLUDES)
LIBDIR = /home/plank/cs360/objs
LIBS = $(LIBDIR)/libfdr.a
EXECUTABLES: jtalk_server jtalk
all: $(EXECUTABLES)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

jtalk_server: jtalk_server.o socketfun.o
		$(CC) $(CFLAGS) -o jtalk_server jtalk_server.o socketfun.o -lpthread $(LIBS)

jtalk: jtalk.o socketfun.o
		$(CC) $(CFLAGS) -o jtalk jtalk.o socketfun.o -lpthread $(LIBS)
clean:
		rm -f core $(EXECUTABLES) jtalk.o jtalk_server.o
