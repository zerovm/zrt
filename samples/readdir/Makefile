NAME=readdir
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
ZVM_ROOT=../..
INCLUDE_FLAGS=-I $(ZVM_ROOT)/lib

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZVM_ROOT)/lib -lzrt
	
$(NAME).o:
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

run:
	./run_tests.sh

clean:
	rm -f some.input
	rm -f $(NAME).nexe $(NAME).o *.log *.data
