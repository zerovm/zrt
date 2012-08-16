NAME=net
PNACL_TOOL=$(shell ../../setupenv.sh)
ZVM_ROOT=../..
INCLUDE_FLAGS=-I $(ZVM_ROOT)/lib
MACROS_FLAGS=-DUSER_SIDE -DZRT_LIB

all: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o $(ZVM_ROOT)/lib/libzrt.a
	@$(ZVM_ROOT)/zvm/zerovm -M$(NAME).manifest -v2

$(NAME).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug
