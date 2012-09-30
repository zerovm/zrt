PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
DEBUG=-DDEBUG
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib

all: createdirs reqrep.nexe run

reqrep.nexe: obj/reqrep.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o reqrep.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/reqrep.o -Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lfs -lstdc++ $(DEBUG)  
	
obj/reqrep.o: reqrep.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/reqrep.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -Wno-long-long -m64 reqrep.c
	
run:
	chmod u+rwx genmanifest.sh test.sh 
	binpatch reqrep.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	./genmanifest.sh
	./test.sh	

createdirs:
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f *.nexe
	@rm -f *.manifest
	

