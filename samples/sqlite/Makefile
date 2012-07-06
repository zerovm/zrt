PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
NAME=sqlexec_ro
NAME1=main_query
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/sqlite3 -I$(ZRT_ROOT)/zvm 

all: $(NAME1).o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME1).o -L$(ZRT_ROOT)/lib -lzrt -L$(ZRT_ROOT)/lib/sqlite3 -lsqlite3  
	@$(ZRT_ROOT)/zvm/zerovm -M$(NAME).manifest -v2

$(NAME1).o: $(NAME1).c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME1).o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 ${NAME1}.c 

clean:
	rm -f $(NAME).nexe *.o *.log *.result
