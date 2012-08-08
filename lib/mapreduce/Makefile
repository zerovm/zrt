NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
DEBUG=-g -DDEBUG
ZRT_ROOT=..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/  -I. 
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: libmapreduce.a libmapreduce_gcc.a

libmapreduce_gcc.a: gcc_buffer.o gcc_map_reduce_lib.o gcc_eachtoother_comm.o gcc_channels_conf.o
	@ar rcs libmapreduce_gcc.a gcc_buffer.o gcc_map_reduce_lib.o gcc_eachtoother_comm.o gcc_channels_conf.o

libmapreduce.a: buffer.o map_reduce_lib.o eachtoother_comm.o channels_conf.o dyn_array.o
	@ar rcs libmapreduce.a buffer.o map_reduce_lib.o eachtoother_comm.o channels_conf.o dyn_array.o


##############################################
############## gcc 
##############################################

gcc_buffer.o: buffer.c
	@gcc -c -o gcc_buffer.o           $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) buffer.c

gcc_map_reduce_lib.o : map_reduce_lib.c map_reduce_lib.h
	@gcc -c -o gcc_map_reduce_lib.o   $(INCLUDE_FLAGS) ${CFLAGS} ${DEBUG} map_reduce_lib.c

gcc_eachtoother_comm.o: eachtoother_comm.c eachtoother_comm.h
	@gcc -c -o gcc_eachtoother_comm.o $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) eachtoother_comm.c

gcc_channels_conf.o: channels_conf.c channels_conf.h
	@gcc -c -o gcc_channels_conf.o    $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) channels_conf.c

##############################################
############## nacl_gcc
##############################################	

buffer.o: buffer.c buffer.h
	@$(NACL_GCC) -c -o buffer.o            $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) buffer.c

map_reduce_lib.o : map_reduce_lib.c map_reduce_lib.h
	@$(NACL_GCC) -c -o map_reduce_lib.o   $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) map_reduce_lib.c

eachtoother_comm.o: eachtoother_comm.c eachtoother_comm.h
	@$(NACL_GCC) -c -o eachtoother_comm.o $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) eachtoother_comm.c

channels_conf.o: channels_conf.c channels_conf.h
	@$(NACL_GCC) -c -o channels_conf.o    $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) channels_conf.c

dyn_array.o: ../helpers/dyn_array.c ../helpers/dyn_array.h
	@$(NACL_GCC) -c -o dyn_array.o    $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) ../helpers/dyn_array.c

clean:
	@rm -f libmapreduce_gcc.a libmapreduce.a *.o 

