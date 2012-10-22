PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CC = $(PNACL_TOOL)/bin/x86_64-nacl-gcc
ifndef TESTS_ROOT
TESTS_ROOT=src
endif
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

#get pipe from zerovm and parse output
PRINT_ERRCODE=awk '{ \
if (NR == 2) { RETCODE=$$1 } \
if (NR == 5) { \
	if (RETCODE == 0) { RESULT="PASS" } \
	else { RESULT="FAIL" } \
	if ($$0 != "ok") { RESULT="FAIL" } \
print RESULT,"zvmstatus=",$$0 } \
}'

TESTBASE=
EXCL=

#CFLAGS+=-O2 #optimization
CFLAGS+=-g #debug
CFLAGS+=-std=gnu99
CFLAGS+=-D_GNU_SOURCE
CFLAGS+=-Wno-long-long -m64
CFLAGS+=-Werror-implicit-function-declaration
CFLAGS+=-I.. -I. -I../api -Isrc
CFLAGS+=-Iinclude
CFLAGS+=-I$(ZRT_ROOT)/lib


#libzrt should be encluded before generic object file of test, because it contains itself main
#declaration, in case if test source didn't explicitly defined zmain it raises linking error;
#in case if libzrt linking after test object file, then linker is using wrong entry point
#if test was not properly ported to zerovm
LDFLAGS_ZMAIN+= -L$(ZRT_ROOT)/lib -lzrt

LDFLAGS+= -m64 -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl
LDFLAGS+= -lfs -lstdc++ 
#For rt tests
LDFLAGS+= -lrt
#For dlopen
LDFLAGS+= -ldl
#math
LDFLAGS+= -lm
#crypt
LDFLAGS+= -lcrypt

################ strings CFLAGS
CFLAGS-inl-tester.c = -fno-builtin
CFLAGS-noinl-tester.c = -fno-builtin
CFLAGS-tst-strlen.c = -fno-builtin
CFLAGS-stratcliff.c = -fno-builtin
CFLAGS-test-ffs.c = -fno-builtin
CFLAGS-tst-inlcall.c = -fno-builtin
################ libio CFLAGS
CFLAGS-tst_putwc.c = -DOBJPFX=\"/tmp/\"
################ math CFLAGS
CFLAGS-test-float.c= -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-double.c= -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-ldouble.c= -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-tgmath.c= -fno-builtin
CFLAGS-test-tgmath2.c= -fno-builtin
CFLAGS-test-tgmath-ret.c= -fno-builtin
# CPPFLAGS was in origin math Makefile
CLAGS-test-ifloat.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH__ \
			 -DTEST_FAST_MATH -fno-builtin
CLAGS-test-idouble.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH__ \
			 -DTEST_FAST_MATH -fno-builtin
CLAGS-test-ildoubl.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH__ \
			  -DTEST_FAST_MATH -fno-builtin
################ posix CFLAGS

################ posix CommandLine
CMDLINE-tst-pcre.c=/tmp/PCRE.tests
CMDLINE-tst-boost.c=/tmp/BOOST.tests

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

$(TEST_NEXES): $(TEST_SOURCES) $(TEST_INCLUDES)
	$(eval BASENAME:=$(basename $@))
	$(eval NAMEONLY:=$(notdir $(BASENAME)))
	$(eval SPECIFIC_TEST_FLAGS:=$(CFLAGS-$(NAMEONLY).c))
	$(eval SPECIFIC_TEST_CMDLINE:=$(CMDLINE-$(NAMEONLY).c))
	@echo -n "TEST RUN $@ "
	@$(CC) -c -o $(BASENAME).o $(CFLAGS) $(SPECIFIC_TEST_FLAGS) $(BASENAME).c
	@$(CC) -o $@ $(LDFLAGS_ZMAIN) $(BASENAME).o $(LDFLAGS)
	@binpatch $@ 4c01fc0f3148c1e2 4c01fc909048c1e2 > /dev/null
	@sed s@{OUTFILE}@$(CURDIR)/$(BASENAME)@g ../manifest_glibc_tests.template | \
	 sed s@{GLIBC_FS}@$(CURDIR)/archive/glibc-fs.tar@g | \
	 sed s@{COMMAND_LINE}@"$(SPECIFIC_TEST_CMDLINE)"@g > $(CURDIR)/$(BASENAME).manifest
#Run test under zerovm and display returned error code
	@setarch x86_64 -R $(ZEROVM) -M$(CURDIR)/$(BASENAME).manifest | $(PRINT_ERRCODE)
