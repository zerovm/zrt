CC=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/bin/x86_64-nacl-gcc
CXX=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/bin/x86_64-nacl-g++

############### libzrt.a source files to build
LIBZRT=lib/libzrt.a
LIBZRT_SOURCES= ${ZEROVM_ROOT}/api/zvm.c lib/syscall_manager.S lib/zrtlog.c \
lib/zrt.c lib/zrtsyscalls.c lib/fs/mounts_manager.c lib/fs/handle_allocator.c \
lib/fs/channels_mount.c lib/fs/channels_readdir.c lib/fs/transparent_mount.c lib/fs/mem_mount_wraper.cc
LIBZRT_OBJECTS=$(addsuffix .o, $(basename $(LIBZRT_SOURCES) ) )

############## ported libraries build
LIBS= lib/mapreduce/libmapreduce.a lib/networking/libnetworking.a \
lib/lua-5.2.1/liblua.a gtest/libgtest.a lib/fs/nacl-mounts/libfs.a lib/sqlite3/libsqlite3.a 

################# samples to build
UNSTABLE_SAMPLES= net
SAMPLES=disort hello readdir reqrep sort_paging time wordcount zshell
TEST_SAMPLES=bigfile command_line environment file_stat seek
 
################# flags set
CFLAGS += -Wall -Wno-long-long -O2 -m64
CFLAGS += -I. -Ilib -Ilib/fs -I${ZEROVM_ROOT}/api
CFLAGS += -DUSER_SIDE -DDEBUG

CXXFLAGS += -I. -Ilib -Ilib/fs

################# "make all" Build libs 
all: prepare ${LIBS} ${LIBZRT} 

prepare:
	@chmod u+rwx ns_start.sh
	@chmod u+rwx ns_stop.sh

${LIBZRT} : $(LIBZRT_OBJECTS)
	$(AR) rcs $@ $(LIBZRT_OBJECTS)

############## Build libs, invoke nested Makefiles
${LIBS}:  
	@make -C$(dir $@)
	@mv -f $@ lib	

############## "make zrt_tests" Build test samples 
zrt_tests: ${TEST_SAMPLES}
${TEST_SAMPLES}: 	
	@make -Ctests/zrt_test_suite/samples/$@

############## "make all_samples" Build samples 
all_samples: ${SAMPLES} 
${SAMPLES}: 
	@make -Csamples/$@

################ "make cleanall" Cleaning libs, tests, samples 	
cleanall: clean clean_samples
	
################ "make clean" Cleaning libs 
LIBS_CLEAN =$(foreach smpl, ${LIBS}, $(smpl).clean)

clean: ${LIBS_CLEAN} #clean_samples 
${LIBS_CLEAN}:
	@make -C$(dir $@) clean 
	@rm -f $(LIBZRT_OBJECTS)
	@rm -f lib/*.a

################ "make clean_samples" Cleaning samples 
SAMPLES_CLEAN =$(foreach smpl, ${SAMPLES}, $(smpl).clean)
TEST_SAMPLES_CLEAN=$(foreach smpl, ${TEST_SAMPLES}, $(smpl).clean)

clean_samples: ${SAMPLES_CLEAN} ${TEST_SAMPLES_CLEAN}
${SAMPLES_CLEAN}:
	@make -Csamples/$(basename $@) clean
${TEST_SAMPLES_CLEAN}:
	@make -Ctests/zrt_test_suite/samples/$(basename $@) clean
		
