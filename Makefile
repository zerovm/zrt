include Makefile.env

############### libzrt.a source files to build
LIBZRT0=lib/libzrt0.a
LIBZRT0_SOURCES= lib/zrt.c

LIBZRT=lib/libzrt.a
LIBZRT_SOURCES= ${ZEROVM_ROOT}/api/zvm.c \
lib/zrtsyscalls.c \
lib/zrtlog.c \
lib/enum_strings.c \
lib/helpers/conf_parser.c \
lib/helpers/conf_keys.c \
lib/helpers/path_utils.c \
lib/memory/memory_syscall_handlers.c \
lib/nvram/nvram_loader.c \
lib/nvram/observers/fstab_observer.c \
lib/fs/fcntl_implem.c \
lib/fs/mounts_manager.c \
lib/fs/handle_allocator.c \
lib/fs/channels_mount.c \
lib/fs/channels_readdir.c \
lib/fs/transparent_mount.c \
lib/fs/mem_mount_wraper.cc \
lib/fs/unpack/stream_reader.c \
lib/fs/unpack/unpack_tar.c \
lib/fs/unpack/image_engine.c \
lib/fs/unpack/parse_path.c

LIBZRT0_OBJECTS=$(addsuffix .o, $(basename $(LIBZRT0_SOURCES) ) )
LIBZRT_OBJECTS=$(addsuffix .o, $(basename $(LIBZRT_SOURCES) ) )


############### zlibc.a source files to build
LIBZGLIBC=lib/libzglibc.a

LIBZGLIBC_SOURCES= lib/glibc_substitute/tmpfile.c \
lib/glibc_substitute/realpath.c \
lib/glibc_substitute/getcwd.c \
lib/glibc_substitute/getuid.c \
lib/glibc_substitute/getpwuid.c \
lib/glibc_substitute/fdopen.c \
lib/glibc_substitute/mkdir.c \
lib/glibc_substitute/rmdir.c \
lib/glibc_substitute/umask.c \
lib/glibc_substitute/unlink.c \
lib/glibc_substitute/chmod.c \
lib/glibc_substitute/fcntl.c \
lib/glibc_substitute/lstat.c \
lib/glibc_substitute/remove.c \
lib/glibc_substitute/rename.c \
lib/glibc_substitute/chown.c \
lib/glibc_substitute/eaccess.c \
lib/glibc_substitute/truncate.c \
lib/glibc_substitute/lockf_stub.c

#lib/glibc_substitute/malloc_free.c lib/memory/bget.c 

LIBZGLIBC_OBJECTS=$(addsuffix .o, $(basename $(LIBZGLIBC_SOURCES) ) )


############## ported libraries build
LIBS= lib/mapreduce/libmapreduce.a lib/networking/libnetworking.a \
lib/lua-5.2.1/liblua.a gtest/libgtest.a lib/fs/nacl-mounts/libfs.a \
lib/tar-1.11.8/libtar.a lib/sqlite3/libsqlite3.a 

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
	-Ilib/nvram \
	-Ilib/nvram/observers \
	-Ilib/fs \
	-Ilib/tar-1.11.8/src \
	-Ilib/fs/unpack \
	-I${ZEROVM_ROOT}/api
CFLAGS += -DDEBUG
CFLAGS += -DUSER_SIDE

CXXFLAGS = -I. -Ilib -Ilib/fs

################# "make all" Build libs 

#debug: LDFLAGS+=-lgcov -fprofile-arcs
#debug: CFLAGS+=-Wdisabled-optimization -fprofile-arcs -ftest-coverage -fdump-rtl-all -fdump-ipa-all 
#debug: prepare ${LIBS} ${LIBZRT} ${LIBZGLIBC} autotests

all: 
all: prepare ${LIBS} ${LIBZRT0} ${LIBZRT} ${LIBZGLIBC} autotests 


#build zrt0 to be used as stub inside of zlibc
zlibc_dep: CFLAGS+=-DZLIBC_STUB
zlibc_dep: cleanzrt0 ${LIBZRT0}


prepare:
	@chmod u+rwx ns_start.sh
	@chmod u+rwx ns_stop.sh

${LIBZRT0} : $(LIBZRT0_OBJECTS)
	$(AR) rcs $@ $(LIBZRT0_OBJECTS)
	@echo $@ updated

${LIBZRT} : $(LIBZRT_OBJECTS)
	$(AR) rcs $@ $(LIBZRT_OBJECTS)
	@echo $@ updated

${LIBZGLIBC} : $(LIBZGLIBC_OBJECTS)
	$(AR) rcs $@ $(LIBZGLIBC_OBJECTS)
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
	@rm -f $(LIBZRT0_OBJECTS)
	@rm -f $(LIBZRT_OBJECTS)
	@rm -f $(LIBZGLIBC_OBJECTS)
	@rm -f $(LIBS)
	@rm -f lib/*.a

cleanzrt0:
	@rm -f ${LIBZRT0}
	@rm -f $(LIBZRT0_OBJECTS)

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

