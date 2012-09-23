PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
NAME=command_line
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib

all: $(NAME).o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe \
	-s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++
	chmod u+rwx genmanifest.sh 
	./genmanifest.sh
	@$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -v2
	cat stdout.data

$(NAME).o:
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
