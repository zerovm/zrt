include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -I. -I..
CFLAGS+=-DDEBUG -g

all: libnetworking.a

libnetworking.a: eachtoother_comm.o channels_conf.o channels_conf_reader.o
	@ar rcs libnetworking.a eachtoother_comm.o channels_conf.o channels_conf_reader.o

clean:
	@rm -f *.a *.o 

