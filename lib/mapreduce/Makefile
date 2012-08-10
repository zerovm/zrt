NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
DEBUG=-g -DDEBUG
ZRT_ROOT=..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/  -I. 
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: libmapreduce.a

libmapreduce.a: buffer.o map_reduce_lib.o
	@ar rcs libmapreduce.a buffer.o map_reduce_lib.o

buffer.o: buffer.c buffer.h
	@$(NACL_GCC) -c -o buffer.o            $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) buffer.c

map_reduce_lib.o : map_reduce_lib.c map_reduce_lib.h
	@$(NACL_GCC) -c -o map_reduce_lib.o   $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) map_reduce_lib.c

clean:
	@rm -f libmapreduce_gcc.a libmapreduce.a *.o 

