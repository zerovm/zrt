PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
NAME=file_stat
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt

$(NAME).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

run: $(NAME).nexe
	echo some data > generated.data
	echo some data > cdr.data
	echo some data > randomread.data
	@$(ZRT_ROOT)/zvm/zerovm -M$(NAME).manifest -v2

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data
