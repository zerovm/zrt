NAME=environment_test
PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
ZRT_ROOT=../../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/gtest

all: $(NAME).nexe

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-g++ -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -L$(ZRT_ROOT)/gtest -lgtest -lzrt   
	@$(ZRT_ROOT)/zvm/zerovm -M$(NAME).manifest -v2

$(NAME).o: $(NAME).cc
	$(PNACL_TOOL)/bin/x86_64-nacl-g++ -c -o $(NAME).o -Wall -static \
	$(INCLUDE_FLAGS) $(DEBUG) -Wno-long-long -O2 -m64 ${NAME}.cc

clean:
	rm -f $(NAME).nexe *.o *.log *.result

