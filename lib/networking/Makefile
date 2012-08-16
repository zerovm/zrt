PNACL_TOOL=$(shell ../../setupenv.sh)
NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
DEBUG=-g -DDEBUG
ZRT_ROOT=..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/  -I. 
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: libnetworking.a

libnetworking.a: eachtoother_comm.o channels_conf.o channels_conf_reader.o dyn_array.o
	@ar rcs libnetworking.a eachtoother_comm.o channels_conf.o channels_conf_reader.o dyn_array.o

eachtoother_comm.o: eachtoother_comm.c eachtoother_comm.h
	@$(NACL_GCC) -c -o eachtoother_comm.o $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) eachtoother_comm.c

channels_conf.o: channels_conf.c channels_conf.h
	@$(NACL_GCC) -c -o channels_conf.o    $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) channels_conf.c

channels_conf_reader.o: channels_conf_reader.c channels_conf_reader.h
	@$(NACL_GCC) -c -o channels_conf_reader.o    $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) channels_conf_reader.c

dyn_array.o: ../helpers/dyn_array.c ../helpers/dyn_array.h
	@$(NACL_GCC) -c -o dyn_array.o    $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) ../helpers/dyn_array.c

clean:
	@rm -f *.a *.o 

