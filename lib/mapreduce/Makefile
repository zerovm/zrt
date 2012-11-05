include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99

all: libmapreduce.a

libmapreduce.a: buffer.o map_reduce_lib.o
	@ar rcs libmapreduce.a buffer.o map_reduce_lib.o

clean:
	@rm -f libmapreduce_gcc.a libmapreduce.a *.o 

