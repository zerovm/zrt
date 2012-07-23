GTEST_CFLAGS=-DGTEST_HAS_PTHREAD=0
DEBUG=-g -DDEBUG
PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I$(ZRT_ROOT)/lib/mapreduce -Isrc
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: createdirs clean gcc_map gcc_reduce map.nexe reduce.nexe

##############################################
############## gcc g++
##############################################
gcc_map: obj/gcc_map.o obj/gcc_common_channels_conf.o obj/gcc_user_implem.o
	gcc -o gcc_map -static obj/gcc_map.o obj/gcc_common_channels_conf.o obj/gcc_user_implem.o \
	-L${ZRT_ROOT}/lib/mapreduce -lmapreduce_gcc $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG)

gcc_reduce: obj/gcc_reduce.o obj/gcc_common_channels_conf.o obj/gcc_user_implem.o
	gcc -o gcc_reduce -static obj/gcc_reduce.o obj/gcc_common_channels_conf.o obj/gcc_user_implem.o \
	-L${ZRT_ROOT}/lib/mapreduce -lmapreduce_gcc $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG)

obj/gcc_map.o: src/map.c
	gcc -c -o obj/gcc_map.o                  $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/map.c

obj/gcc_reduce.o: src/reduce.c
	gcc -c -o obj/gcc_reduce.o               $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/reduce.c	

obj/gcc_common_channels_conf.o: src/common_channels_conf.c src/common_channels_conf.h
	gcc -c -o obj/gcc_common_channels_conf.o $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/common_channels_conf.c

obj/gcc_user_implem.o : src/user_implem.c src/user_implem.h
	gcc -c -o obj/gcc_user_implem.o $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) -D_GNU_SOURCE src/user_implem.c

##############################################
############## nacl_gcc nacl_g++
##############################################	
map.nexe: obj/map.o obj/common_channels_conf.o obj/user_implem.o
	$(NACL_GCC) -o map.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/map.o obj/common_channels_conf.o obj/user_implem.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lmapreduce $(DEBUG)  
	
reduce.nexe: obj/reduce.o obj/common_channels_conf.o obj/user_implem.o
	$(NACL_GCC) -o reduce.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/reduce.o obj/common_channels_conf.o obj/user_implem.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lmapreduce $(DEBUG)  	

obj/map.o: src/map.c
	$(NACL_GCC) -c -o obj/map.o                  $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/map.c -DUSER_SIDE 

obj/reduce.o: src/reduce.c
	$(NACL_GCC) -c -o obj/reduce.o               $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/reduce.c -DUSER_SIDE

obj/common_channels_conf.o: src/common_channels_conf.c src/common_channels_conf.h
	$(NACL_GCC) -c -o obj/common_channels_conf.o $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/common_channels_conf.c -DUSER_SIDE
	
obj/user_implem.o: src/user_implem.c src/user_implem.h
	$(NACL_GCC) -c -o obj/user_implem.o          $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/user_implem.c -D_GNU_SOURCE

createdirs:
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f *.nexe gcc_map gcc_reduce
	

