PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

CC=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/bin/x86_64-nacl-gcc

#DEBUG=-g
CFLAGS += -I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/lib/networking -I.
CFLAGS += -Wall -std=c99 -Wno-long-long -O2 -msse4.1 -m64 $(DEBUG)
LDFLAGS += -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl \
-L$(ZRT_ROOT)/lib -lzrt -lnetworking -lfs -lstdc++

GENNODE=generator.uint32_t
MANNODE=nodeman
SRCNODE=nodesrc
DSTNODE=nodedst

NEXES = $(GENNODE).nexe $(MANNODE).nexe $(SRCNODE).nexe $(DSTNODE).nexe
GENNODE_OBJECTS = generator.uint32_t.o
MANNODE_OBJECTS = comm.o bitonic_sort.o manager/main_man.o manager/histanlz.o manager/comm_man.o dsort.o sort.o
SRCNODE_OBJECTS = comm.o source/comm_src.o source/main_src.o cpuid.o bitonic_sort.o dsort.o sort.o
DSTNODE_OBJECTS = dest/main_dst.o dest/comm_dst.o comm.o dsort.o sort.o

all: cleanlog prepare $(NEXES) 

$(GENNODE).nexe: $(GENNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	binpatch $(GENNODE).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

$(MANNODE).nexe: $(MANNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	binpatch $(MANNODE).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

$(SRCNODE).nexe: $(SRCNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	binpatch $(SRCNODE).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

$(DSTNODE).nexe: $(DSTNODE_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	binpatch $(DSTNODE).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

cleanlog:
	@rm -f log/*.log
	
prepare: createdirs
	@chmod u=rwx disort.sh
	@chmod u=rwx gendata.sh
	@chmod u=rwx genmanifest.sh
	@./genmanifest.sh
 	
createdirs:
	@mkdir -p log data

clean: cleanlog 
	@rm -f $(NEXES)
	@rm -f $(GENNODE_OBJECTS) $(MANNODE_OBJECTS) $(SRCNODE_OBJECTS) $(DSTNODE_OBJECTS)
	@rm -f manifest/*.manifest
	@rm -f data/*.data


