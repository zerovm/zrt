NAME=manifest_test
PNACL_TOOL=$(shell ../../setupenv.sh)
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I.

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt    

$(NAME).o: $(NAME).c
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 ${NAME}.c 

run:
	@./run_tests.sh

clean:
	rm -f $(NAME).nexe *.o *.log *.result *.data
	rm -f *.manifest

