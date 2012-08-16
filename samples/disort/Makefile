PNACL_TOOL=$(shell ../../setupenv.sh)
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/networking -I$(ZRT_ROOT)/zvm -I.
#DEBUG=-g
GENNODE=generator.uint32_t
MANNODE=nodeman
SRCNODE=nodesrc
DSTNODE=nodedst

all: cleanlog prepare $(GENNODE).nexe $(MANNODE).nexe $(SRCNODE).nexe $(DSTNODE).nexe

$(GENNODE).nexe : obj/$(GENNODE).o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(GENNODE).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) obj/$(GENNODE).o -L$(ZRT_ROOT)/lib -lzrt
	
$(MANNODE).nexe: obj/main_man.o obj/comm_man.o obj/histanlz.o obj/dsort.o obj/comm.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(MANNODE).nexe  -s -static -T $(DEBUG) \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_man.o obj/comm_man.o obj/histanlz.o obj/dsort.o obj/comm.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lnetworking
	
$(SRCNODE).nexe: obj/main_src.o obj/sort.o obj/dsort.o obj/comm_src.o obj/bitonic.o obj/cpuid.o obj/comm.o 
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(SRCNODE).nexe  -s -static -T $(DEBUG) \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_src.o obj/sort.o obj/dsort.o obj/comm_src.o obj/bitonic.o obj/cpuid.o obj/comm.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lnetworking
	  
$(DSTNODE).nexe: obj/main_dst.o obj/sort.o obj/dsort.o obj/comm_dst.o obj/comm.o 
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o $(DSTNODE).nexe  -s -static -T $(DEBUG) \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_dst.o obj/sort.o obj/dsort.o obj/comm_dst.o obj/comm.o  \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -lnetworking

#source node & dest node
obj/dsort.o: dsort.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/dsort.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 dsort.c

#source node & dest node
obj/sort.o: sort.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/sort.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 sort.c

#destination node
obj/main_dst.o: dest/main_dst.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/main_dst.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Idest dest/main_dst.c

#destination node
obj/comm_dst.o: dest/comm_dst.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/comm_dst.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Idest dest/comm_dst.c

#source node
obj/main_src.o: source/main_src.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/main_src.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Isource source/main_src.c
	
#source node
obj/comm_src.o: source/comm_src.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/comm_src.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Isource source/comm_src.c

#manager node
obj/main_man.o: manager/main_man.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/main_man.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Isource manager/main_man.c

#manager node
obj/comm_man.o: manager/comm_man.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/comm_man.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Imanager manager/comm_man.c

#manager node
obj/histanlz.o: manager/histanlz.c defines.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/histanlz.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Imanager manager/histanlz.c

#all nodes
obj/comm.o : comm.c comm.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/comm.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -m64 -Imanager comm.c

obj/bitonic.o: bitonic_sort.c bitonic_sort.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/bitonic.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Imanager bitonic_sort.c

obj/cpuid.o : cpuid.c cpuid.h
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/cpuid.o -Wall -std=c99 \
	$(INCLUDE_FLAGS)  $(DEBUG) -Wno-long-long -O2 -m64 -Imanager cpuid.c

obj/$(GENNODE).o : $(GENNODE).c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o obj/$(GENNODE).o -Wall \
	$(INCLUDE_FLAGS)  $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(GENNODE).c

cleanlog:
	@rm -f log/*.log
	
prepare: createdirs
	@chmod u=rwx gendata.sh
	@chmod u=rwx genmanifest.sh
	@./genmanifest.sh
 	
createdirs:
	@mkdir -p obj log data vmlog

clean: cleanlog 
	@rm -f *.nexe
	@rm -f obj/*.o
	@rm -f manifest/*.manifest
	@rm -f data/*.data
	@rm -f $(MANNODE).nexe $(SRCNODE).nexe $(DSTNODE).nexe
	

