GTEST_CFLAGS=-DGTEST_HAS_PTHREAD=0
DEBUG=-g -DDEBUG
PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I$(ZRT_ROOT)/lib/lua-5.2.1/src -I.
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: createdirs clean zshell.nexe

zshell.nexe: luaexec.o main.o
	$(NACL_GCC) -o zshell.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 luaexec.o main.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -llua -lm $(DEBUG)  
	
main.o: main.c
	$(NACL_GCC) -c -o main.o     $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) main.c -DUSER_SIDE 

luaexec.o: luaexec.c
	$(NACL_GCC) -c -o luaexec.o  $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) luaexec.c -DUSER_SIDE

createdirs:
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f zshell.nexe
	

