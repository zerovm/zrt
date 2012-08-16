NAME=readdir
PNACL_TOOL=$(shell ../../setupenv.sh)
ZVM_ROOT=../..
INCLUDE_FLAGS=-I $(ZVM_ROOT)/lib

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZVM_ROOT)/lib -lzrt
	
$(NAME).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

run:
	./run_tests.sh

clean:
	@rm -f some.input
	@rm -f $(NAME).nexe $(NAME).o *.log *.data
