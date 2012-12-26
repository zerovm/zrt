PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CC = $(PNACL_TOOL)/bin/x86_64-nacl-gcc

#Note1: SPECIFY TESTS FOLDER
#TESTS_ROOT variable may be defined externally containing related path to tests folder
#if it's not defined then would be used TESTS_ROOT defined below in Makefile
ifndef TESTS_ROOT
TESTS_ROOT=autotests
endif
ifndef MEMMAX
MEMMAX=234217728
endif

#Note2: RELEASE VERSION OF ZEROVM
#Given Makefile - test runner is using release build of zerovm, and it should be
#accessible at path described just here:
ZEROVM=$(ZEROVM_ROOT)/zerovm.rel

OUTPUT_LAST=last_test.output
OUTPUT_FAIL=fail_tests.output
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
COUNTER=0

#get pipe from zerovm and parse output
PRINT_ERRCODE=awk '{ \
if (NR == 2) { RETCODE=$$1 } \
if (NR == 5) { \
	if (RETCODE == 0) { RESULT="PASS" } \
	else { RESULT="FAIL" } \
	if ($$0 != "ok") { RESULT="FAIL" } \
print RESULT,"zvmstatus=",$$0 } \
}'

DISPLAY_FAIL=awk '{\
if ( match($$0, "FAIL zvmstatus") != 0 ) {\
	print $$0\
	}\
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
#if test was not properly ported to zerovm and didn't have zmain function
LDFLAGS_ZMAIN+= -L$(ZRT_ROOT)/lib -lzrt

LDFLAGS+= -m64 -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl
#in order to correectly link glibs substituted functions libzrt should be linked twice
#before object file and after
LDFLAGS+= -lfs -lzrt -lzglibc -lstdc++ 
#For rt tests
LDFLAGS+= -lrt
#For dlopen
LDFLAGS+= -ldl
#math
LDFLAGS+= -lm
#crypt
LDFLAGS+= -lcrypt

#####################################################################
#in this section describe compiler flags by defining makefile variable 
#CFLAGS-xxxxxxxx= gcc flags listed here
#where xxxxxxxx is name of source file which should be compiled with additional flags
#examples: 
#CFLAGS-inl-tester.c = -fno-builtin
#CLAGS-test-ifloat.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH_
#####################################################################

#####################################################################
#in this section specify command line arguments which should be passed 
#as cmd line parameters for nexe, it should be specified in format:
#CMDLINE-xxxxxxxxx=cmd line argument listed here
#where xxxxxxxxx is the source file name waiting cmd line arguments
#examples:
#CMDLINE-tst-pcre.c=arg1 arg2 arg3
#CMDLINE-tst-boost.c=/tmp/BOOST.tests
CMDLINE-command_line.c=argument_number_1 argument_number_2 the_last_argument
#####################################################################

#####################################################################
#in this section describe environment variables which should be set in manifest environment
#ENV-xxxxxxxx=, envname, envvalue, envname2, envvalue2
#xxxxxxx - source file name, just after '=' should be passed environment variable names
#and their value divided by comma ',', and not that just after '=' is expected comma ','
#examples: 
#ENV-tst-swscanf.c=, LOCPATH, /lib/locale
ENV-environment.c=, SafeWords, klaato_verada_nikto
#####################################################################

all: clean $(ZEROVM) $(TEST_NEXES) report

resume:$(ZEROVM) $(TEST_NEXES)

clean:
	@rm -f *.o
	@rm -f $(OUTPUT_LAST)
	@rm -f $(OUTPUT_FAIL)
	@rm -f $(TEST_OBJECTS) 
	@rm -f $(TEST_STDOUTS)
	@rm -f $(TEST_STDERRS)
	@rm -f $(TEST_LOG_ZVM)
	@rm -f $(TEST_NEXES)
	@rm -f $(TEST_MANIFESTS)
	@rm -f $(TEST_LOG_DEBUG)

$(ZEROVM):
	$(error "$(ZEROVM) does not exist")

$(TEST_NEXES): $(TEST_SOURCES) $(TEST_INCLUDES)
#increase test counter
	$(eval COUNTER:=$(shell echo $$(($(COUNTER)+1))) )
#prrepare variables
	$(eval BASENAME:=$(basename $@))
	$(eval NAMEONLY:=$(notdir $(BASENAME)))
	$(eval SPECIFIC_TEST_FLAGS:=$(CFLAGS-$(NAMEONLY).c))
	$(eval SPECIFIC_TEST_CMDLINE:=$(CMDLINE-$(NAMEONLY).c))
	$(eval SPECIFIC_TEST_ENV:=$(ENV-$(NAMEONLY).c))
	@echo -n "TEST RUN $@ "
#compile
	@$(CC) -c -o $(BASENAME).o $(CFLAGS) $(SPECIFIC_TEST_FLAGS) $(BASENAME).c
	@$(CC) -o $@ $(LDFLAGS_ZMAIN) $(BASENAME).o $(LDFLAGS)
#prepare binary
	@binpatch $@ 4c01fc0f3148c1e2 4c01fc909048c1e2 > /dev/null
#prepare manifest
	@sed s@{OUTFILE}@$(CURDIR)/$(BASENAME)@g manifest_zrttests.template | \
	 sed s@{ABS_PATH}@$(CURDIR)/@g | \
	 sed s@{MOUNTS_PATH}@$(ZRT_ROOT)/mounts/@g | \
	 sed s@{ENVIRONMENT}@"$(SPECIFIC_TEST_ENV)"@g | \
	 sed s@{MEMMAX}@$(MEMMAX)@g | \
	 sed s@{COMMAND_LINE}@"$(SPECIFIC_TEST_CMDLINE)"@g > $(CURDIR)/$(BASENAME).manifest
#Run test under zerovm and display returned error code
#$shell should could not be used to get test result from file because it unexpectadly
#using contents of previosly saved test output, and it is the main reason why using awk;
#parsing correct output retrieved via pipe
	@setarch x86_64 -R $(ZEROVM) -M$(CURDIR)/$(BASENAME).manifest \
	| $(PRINT_ERRCODE) > $(OUTPUT_LAST)
	@cat $(OUTPUT_LAST)
#save failed tests to displaytheir at report
	@cat $(OUTPUT_LAST) | awk '{\
if ( match($$0, "FAIL zvmstatus") != 0 ) {\
	print test,$$0\
	}\
}' test="$@" >> $(OUTPUT_FAIL)

report: $(OUTPUT_FAIL)
#display tests resuts
	@echo --------------------------------------------
	@echo $(COUNTER) tests was executed
	@echo -n "Failed "
	@echo -n `cat $(OUTPUT_FAIL) | wc -l`
	@echo " tests:"
	@cat $(OUTPUT_FAIL)
	@rm -f $(OUTPUT_FAIL) $(OUTPUT_LAST)

$(OUTPUT_FAIL):
#has no output fail if this rule is active
	$(error Nothing output)
