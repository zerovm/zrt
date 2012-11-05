include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -I. -I..
CFLAGS+=-DDEBUG -g

all: libnetworking.a

libnetworking.a: eachtoother_comm.o channels_conf.o channels_conf_reader.o dyn_array.o
	@ar rcs libnetworking.a eachtoother_comm.o channels_conf.o channels_conf_reader.o dyn_array.o

dyn_array.o: ../helpers/dyn_array.c ../helpers/dyn_array.h
	@$(CC) -c -o dyn_array.o ${CFLAGS} ../helpers/dyn_array.c

clean:
	@rm -f *.a *.o 

