include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -DDEBUG

all: libmapreduce.a

libmapreduce.a: buffer.o buffered_io.o map_reduce_lib.o
	@ar rcs libmapreduce.a buffer.o buffered_io.o map_reduce_lib.o

clean:
	@rm -f libmapreduce.a *.o 

