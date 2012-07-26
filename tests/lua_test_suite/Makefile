NAME=lua
PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
ZRT_ROOT=../..
SRC_PATH=$(ZRT_ROOT)/lib/lua-5.2.1/src/not_for_lib
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/lua-5.2.1/src 

all: $(NAME).nexe

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt -llua -lm    
	@$(ZRT_ROOT)/zvm/zerovm -M$(NAME).manifest -v2

$(NAME).o: $(SRC_PATH)/$(NAME).c
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 $(SRC_PATH)/${NAME}.c 

clean:
	rm -f $(NAME).nexe *.o *.log *.result

