PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CC = $(PNACL_TOOL)/bin/x86_64-nacl-gcc
TESTS_ROOT=src
#it should be used release version of zerovm
ZEROVM=$(ZEROVM_ROOT)/zerovm.rel
ZEROVM_LOG=zerovm.log

TEST_INCLUDES=$(shell find $(TESTS_ROOT) -name "*.h")
TEST_SOURCES=$(shell find $(TESTS_ROOT) -name "*.c")
TEST_OBJECTS=$(addsuffix .o, $(basename $(TEST_SOURCES) ) )
TEST_MANIFESTS=$(addsuffix .manifest, $(basename $(TEST_SOURCES) ) )
TEST_STDOUTS=$(addsuffix .stdout.std, $(basename $(TEST_SOURCES) ) )
TEST_STDERRS=$(addsuffix .stderr.std, $(basename $(TEST_SOURCES) ) )
TEST_LOG_ZVM=$(addsuffix .zerovm.log, $(basename $(TEST_SOURCES) ) )
TEST_LOG_DEBUG=$(addsuffix .zrtdebug.log, $(basename $(TEST_SOURCES) ) )
TEST_NEXES=$(patsubst %.o, %.nexe, $(TEST_OBJECTS))
TEST_LIST=$(basename $(TEST_SOURCES) )


TESTBASE=
EXCL=

#CFLAGS+=-std=c99
CFLAGS+=-D__USE_GNU
CFLAGS+=-Wno-long-long -O2 -msse4.1 -m64 -Wall
CFLAGS+=-Isrc
CFLAGS+=-Iapi
CFLAGS+=-I$(ZRT_ROOT)/lib


all: prepare clean $(ZEROVM) $(TEST_LIST)

prepare:
	@cat /dev/null > $(ZEROVM_LOG)

clean:
	@rm -f *.o $(TEST_OBJECTS) $(TEST_STDOUTS) $(TEST_STDERRS) $(TEST_LOG_ZVM) \
$(TEST_NEXES) $(TEST_MANIFESTS) $(TEST_LOG_DEBUG)

$(ZEROVM):
	@echo "$(ZEROVM) does not exist, and it should be compiled for Release"
	@exit 1

$(TEST_LIST): $(TEST_INCLUDES)
	$(eval EXCL := $(strip $(shell grep $@ excluded.txt)))
	@echo -n "TEST RUN $@ retcode="
	@$(CC) -c -o $@.o $(CFLAGS) $@.c
	@$(CC) -o $@.nexe -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 -fPIC $@.o -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++
	@binpatch $@.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2 > /dev/null
	@sed s@{OUTFILE}@$(CURDIR)/$@@g manifest_glibc_tests.template | \
	 sed s@{GLIBC_FS}@$(ZRT_ROOT)/archive/lib.tar@g > $(CURDIR)/$@.manifest
#Run test under zerovm and display returned error code
	@setarch x86_64 -R $(ZEROVM) -M$(CURDIR)/$@.manifest -v2 | awk '{if (NR == 2) { print }}'
