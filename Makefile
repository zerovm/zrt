CC=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/bin/x86_64-nacl-gcc
CXX=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/bin/x86_64-nacl-g++
AR=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/bin/x86_64-nacl-ar

############### libzrt.a source files to build
LIBZRT=lib/libzrt.a
LIBZRT_SOURCES= ${ZEROVM_ROOT}/api/zvm.c \
lib/syscall_manager.S \
lib/zrtlog.c \
lib/zrt.c \
lib/zrtsyscalls.c \
lib/enum_strings.c \
lib/helpers/conf_parser.c \
lib/memory/memory.c \
lib/glibc_substitute/tmpfile.c \
lib/fs/fcntl_implem.c \
lib/fs/fstab_loader.c \
lib/fs/fstab_observer.c \
lib/fs/mounts_manager.c \
lib/fs/handle_allocator.c \
lib/fs/channels_mount.c \
lib/fs/channels_readdir.c \
lib/fs/transparent_mount.c \
lib/fs/mem_mount_wraper.cc \
lib/fs/unpack/stream_reader.c \
lib/fs/unpack/unpack_tar.c \
lib/fs/unpack/image_engine.c \
lib/fs/utils/parse_path.c

LIBZRT_OBJECTS=$(addsuffix .o, $(basename $(LIBZRT_SOURCES) ) )

############## ported libraries build
LIBS= lib/mapreduce/libmapreduce.a lib/networking/libnetworking.a \
lib/lua-5.2.1/liblua.a gtest/libgtest.a lib/fs/nacl-mounts/libfs.a lib/sqlite3/libsqlite3.a 

################# samples to build
UNSTABLE_SAMPLES=
SAMPLES=hello tarimage readdir sort_paging reqrep disort wordcount zshell time
TEST_SAMPLES=file_stat bigfile
TEST_SUITES=lua_test_suite

################# flags set
CFLAGS = -Wall -Wno-long-long -O2 -m64
CFLAGS += -Werror-implicit-function-declaration
#include paths
CFLAGS += -I. \
	-Ilib \
	-Ilib/memory \
	-Ilib/helpers \
	-Ilib/fs \
	-Ilib/fs/unpack \
	-Ilib/fs/utils \
	-I${ZEROVM_ROOT}/api
CFLAGS += -DDEBUG
CFLAGS += -DUSER_SIDE

CXXFLAGS = -I. -Ilib -Ilib/fs

################# "make all" Build libs 
all: prepare ${LIBS} ${LIBZRT} autotests

prepare:
	@chmod u+rwx ns_start.sh
	@chmod u+rwx ns_stop.sh

${LIBZRT} : $(LIBZRT_OBJECTS)
	$(AR) rcs $@ $(LIBZRT_OBJECTS)
	@echo $@ updated

############## Build libs, invoke nested Makefiles
${LIBS}:  
	@make -C$(dir $@)
	@echo move $@ library to final folder
	@mv -f $@ lib	

############## "make test" Build & Run all tests
test: test_suites zrt_tests

############## "make autotests" run zrt autotests
autotests:
	@echo ------------- RUN zrt $@ ------------
	@TESTS_ROOT=$@ make -Ctests/zrt_test_suite

############## "make zrt_tests" Build test samples 
test_suites: ${TEST_SUITES}
${TEST_SUITES}: 	
	@make -Ctests/$@

############## "make zrt_tests" Build test samples 
zrt_tests: ${TEST_SAMPLES}
${TEST_SAMPLES}: 	
	@make -Ctests/zrt_test_suite/functional/$@

############## "make all_samples" Build samples 
all_samples: ${SAMPLES} 
${SAMPLES}: 
	@make -Csamples/$@

################ "make cleanall" Cleaning libs, tests, samples 	
cleanall: clean clean_samples clean_test_suites

################ "make clean" Cleaning libs 
LIBS_CLEAN =$(foreach smpl, ${LIBS}, $(smpl).clean)

clean: ${LIBS_CLEAN}  
${LIBS_CLEAN}:
	@make -C$(dir $@) clean 
	@TESTS_ROOT=autotests make -Ctests/zrt_test_suite clean
	@rm -f $(LIBZRT_OBJECTS)
	@rm -f $(LIBS)
	@rm -f lib/*.a

################ "make clean_samples" Cleaning samples 
SAMPLES_CLEAN =$(foreach smpl, ${SAMPLES}, $(smpl).clean)
TEST_SAMPLES_CLEAN=$(foreach smpl, ${TEST_SAMPLES}, $(smpl).clean)

clean_samples: ${SAMPLES_CLEAN} ${TEST_SAMPLES_CLEAN}
${SAMPLES_CLEAN}:
	@make -Csamples/$(basename $@) clean
${TEST_SAMPLES_CLEAN}:
	@make -Ctests/zrt_test_suite/functional/$(basename $@) clean

################ "make clean_test_suites" Cleaning test suites
TESTS_CLEAN=$(foreach suite, ${TEST_SUITES}, $(suite).clean)

clean_test_suites: ${TESTS_CLEAN}
${TESTS_CLEAN}:
	@make -Ctests/$(basename $@) clean

