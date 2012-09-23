GTEST_CFLAGS=-DGTEST_HAS_PTHREAD=0
DEBUG=-g -DDEBUG
PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/mapreduce -I$(ZRT_ROOT)/lib/networking -Isrc
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: createdirs clean map.nexe reduce.nexe

map.nexe: obj/map.o obj/user_implem.o
	@$(NACL_GCC) -o map.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 $(DEBUG) \
	obj/map.o obj/user_implem.o -Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lfs -lmapreduce -lnetworking -lstdc++   
	
reduce.nexe: obj/reduce.o obj/user_implem.o
	@$(NACL_GCC) -o reduce.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 $(DEBUG) \
	obj/reduce.o obj/user_implem.o -Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lfs -lmapreduce -lnetworking -lstdc++   	

obj/map.o: src/map.c
	@$(NACL_GCC) -c -o obj/map.o                  $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/map.c -DUSER_SIDE 

obj/reduce.o: src/reduce.c
	@$(NACL_GCC) -c -o obj/reduce.o               $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/reduce.c -DUSER_SIDE

obj/user_implem.o: src/user_implem.c src/user_implem.h
	@$(NACL_GCC) -c -o obj/user_implem.o          $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) src/user_implem.c -D_GNU_SOURCE

createdirs:
	chmod u+rwx genmanifest.sh mapred_conf.sh mr_start.sh
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f data/*.res
	@rm -f manifest/*.manifest	
	@rm -f *.nexe gcc_map gcc_reduce
	

