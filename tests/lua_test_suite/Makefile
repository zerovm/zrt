NAME=lua
PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
SRC_PATH=$(ZRT_ROOT)/lib/lua-5.2.1/src/not_for_lib
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/lua-5.2.1/src 

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt -llua -lm -lfs -lstdc++  
	binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

$(NAME).o: $(SRC_PATH)/$(NAME).c
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 $(SRC_PATH)/${NAME}.c 

run:
	chmod u+rwx run_tests.sh
	@./run_tests.sh

clean:
	rm -f $(NAME).nexe *.o *.log *.result

