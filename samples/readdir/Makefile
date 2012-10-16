NAME=readdir
PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc
INCLUDE_FLAGS=-I $(ZRT_ROOT)/lib

all: $(NAME).nexe run 

$(NAME).nexe: $(NAME).o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++
	@binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

$(NAME).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

run:
	chmod u+rwx run_tests.sh 
	./run_tests.sh

clean:
	@rm -f some.input
	@rm -f $(NAME).nexe $(NAME).o *.log *.data
