PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/
NAME_1=generator.uint32_t
NAME_2=sort_uint_proper_with_args
NAME_3=tester.uint32_t
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm

all: $(NAME_1).o $(NAME_2).o $(NAME_3).o cpuid.o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME_1).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME_1).o -L$(ZRT_ROOT)/lib -lzrt  -lfs -lstdc++
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME_2).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME_2).o cpuid.o -L$(ZRT_ROOT)/lib -lzrt  -lfs -lstdc++
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME_3).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME_3).o -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++
#
	binpatch $(NAME_1).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	binpatch $(NAME_2).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	binpatch $(NAME_3).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
#
	chmod u+rwx run_test.sh genmanifest.sh
	./run_test.sh

$(NAME_1).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME_1).o -Wall \
	$(INCLUDE_FLAGS)  $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME_1).c

$(NAME_2).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME_2).o -Wall \
	$(INCLUDE_FLAGS)  $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME_2).c

$(NAME_3).o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(NAME_3).o -Wall \
	$(INCLUDE_FLAGS)  $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME_3).c

cpuid.o:
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o cpuid.o -Wall \
	$(INCLUDE_FLAGS)  $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 cpuid.c

clean:
	@rm -f *.nexe *.o *.log *.data *.manifest
