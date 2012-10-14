PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CC = $(PNACL_TOOL)/bin/x86_64-nacl-gcc
TESTS_ROOT=src
#it should be used release version of zerovm
ZEROVM=$(ZEROVM_ROOT)/zerovm.rel

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

CFLAGS+=-std=gnu99
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-Wno-long-long -O2 -msse4.1 -m64
CFLAGS+=-I.. -I. -I../api -Isrc
CFLAGS+=-Iinclude
CFLAGS+=-I$(ZRT_ROOT)/lib
#CFLAGS+=-Isrc/rt

LDFLAGS+= -m64 -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl
LDFLAGS+= -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++ 
#For rt tests
LDFLAGS+= -lrt
#For dlopen
LDFLAGS+= -ldl


all: clean $(ZEROVM) $(TEST_NEXES)

resume:$(ZEROVM) $(TEST_NEXES)

clean:
	@rm -f *.o
	@rm -f $(TEST_OBJECTS) 
	@rm -f $(TEST_STDOUTS)
	@rm -f $(TEST_STDERRS)
	@rm -f $(TEST_LOG_ZVM)
	@rm -f $(TEST_NEXES)
	@rm -f $(TEST_MANIFESTS)
	@rm -f $(TEST_LOG_DEBUG)

$(ZEROVM):
	@echo "$(ZEROVM) does not exist, and it should be compiled for Release"
	@exit 1

$(TEST_NEXES):  $(TEST_INCLUDES)
	$(eval BASENAME:=$(basename $@))
	@echo -n "TEST RUN $@ "
	@$(CC) -c -o $(BASENAME).o $(CFLAGS) $(BASENAME).c
	@$(CC) -o $@ $(BASENAME).o $(LDFLAGS)
	@binpatch $@ 4c01fc0f3148c1e2 4c01fc909048c1e2 > /dev/null
	@sed s@{OUTFILE}@$(CURDIR)/$(BASENAME)@g ../manifest_glibc_tests.template | \
	 sed s@{GLIBC_FS}@$(ZRT_ROOT)/archive/lib.tar@g > $(CURDIR)/$(BASENAME).manifest
#Run test under zerovm and display returned error code
	@echo -n "retcode="
	@setarch x86_64 -R $(ZEROVM) -M$(CURDIR)/$(BASENAME).manifest -v2 | awk '{if (NR == 2) { print }}'
