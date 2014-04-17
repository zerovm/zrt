include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -DDEBUG

all: libmapreduce.a

#use macros BASEFILE__ if no need full srcpath in log debug file
%.o: %.c
	$(CC) $(CFLAGS) -DBASEFILE__=\"$(notdir $<)\" $< -o $@

libmapreduce.a: $(CURDIR)/buffer.o $(CURDIR)/map_reduce_lib.o
	@ar rcs libmapreduce.a buffer.o map_reduce_lib.o

clean:
	@rm -f libmapreduce.a *.o 

