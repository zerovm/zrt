PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
NAME=luaexec
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/lua-5.2.1/src -I$(ZRT_ROOT)/zvm 

all: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt -llua -lm    
	@$(ZRT_ROOT)/zvm/zerovm -M$(NAME).manifest -v2

$(NAME).o: $(NAME).c
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 ${NAME}.c 

clean:
	rm -f $(NAME).nexe *.o *.log *.result
