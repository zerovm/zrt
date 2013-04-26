include Makefile.env

############### libzrt.a source files to build
LIBDEP_SOURCES= lib/zrt.c

LIBZRT=lib/libzrt.a
LIBZRT_SOURCES= \
lib/zcalls/zcalls_prolog.c \
lib/zcalls/zcalls_zrt.c \
lib/libc/loglibc.c \
lib/libc/fcntl.c \
lib/libc/link.c \
lib/libc/unlink.c \
lib/libc/rmdir.c \
lib/libc/mkdir.c \
lib/libc/chmod.c \
lib/libc/chown.c \
lib/libc/ftruncate.c \
lib/zrtlog.c \
lib/enum_strings.c \
lib/helpers/printf_prolog.c \
lib/helpers/conf_parser.c \
lib/helpers/conf_keys.c \
lib/helpers/path_utils.c \
lib/memory/memory_syscall_handlers.c \
lib/nvram/nvram_loader.c \
lib/nvram/observers/environment_observer.c \
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
lib/fs/unpack/parse_path.c \
lib/zrt.c

LIBDEP_OBJECTS=$(addsuffix .o, $(basename $(LIBDEP_SOURCES) ) )
LIBZRT_OBJECTS=$(addsuffix .o, $(basename $(LIBZRT_SOURCES) ) )


############## zrtlibs and ported libraries build
LIBS= \
lib/mapreduce/libmapreduce.a \
lib/networking/libnetworking.a \
lib/fs/nacl-mounts/libfs.a 

LIBPORTS= \
libports/gtest/libgtest.a \
libports/lua-5.2.1/liblua.a \
libports/tar-1.11.8/libtar.a \
libports/sqlite3/libsqlite3.a 

################# samples to build
UNSTABLE_SAMPLES=
SAMPLES=hello tarimage readdir sort_paging reqrep disort wordcount zshell time cgioutput
TEST_SAMPLES=file_stat bigfile
TEST_SUITES=lua_test_suite

################# flags set
CFLAGS += -Werror-implicit-function-declaration
CFLAGS += -DDEBUG
CFLAGS += -DUSER_SIDE

################# "make all" Build libs 

#debug: LDFLAGS+=-lgcov -fprofile-arcs
#debug: CFLAGS+=-Wdisabled-optimization -fprofile-arcs -ftest-coverage -fdump-rtl-all -fdump-ipa-all 
#debug: prepare ${LIBS} ${LIBZRT} autotests

all: notests autotests

notests: doc ${LIBS} ${LIBPORTS} ${LIBDEP_OBJECTS} ${LIBZRT}

#build zrt0 to be used as stub inside of zlibc
zlibc_dep: CFLAGS+=-DZLIBC_STUB
zlibc_dep: ${LIBDEP_OBJECTS}

${LIBZRT} : $(LIBZRT_OBJECTS)
	$(AR) rcs $@ $(LIBZRT_OBJECTS)
	@echo $@ updated

############## Build libs, invoke nested Makefiles
${LIBS}:
	@make -C$(dir $@)
	@echo move $@ library to final folder
	@mv -f $@ lib	

${LIBPORTS}:
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

################ "make clean" Cleaning libs, tests, samples 	
clean: libclean clean_ports clean_test_suites
	@rm -f lib/*.a

################ "make clean" Cleaning libs 
LIBS_CLEAN =$(foreach smpl, ${LIBS}, $(smpl).clean)
LIBPORTS_CLEAN =$(foreach smpl, ${LIBPORTS}, $(smpl).clean)

libclean: ${LIBS_CLEAN}
${LIBS_CLEAN}: cleandep
	@rm -f $(LIBZRT_OBJECTS)
	@rm -f $(LIBS) $(LIBZRT)
	@make -C$(dir $@) clean 
	@TESTS_ROOT=autotests make -Ctests/zrt_test_suite clean

clean_ports: ${LIBPORTS_CLEAN}
${LIBPORTS_CLEAN}:
	@make -C$(dir $@) clean 
	@rm -f $(LIBPORTS)

cleandep:
	@rm -f ${LIBDEP_OBJECTS}

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

################ "make doc" Generate doc file concatenating all READMEs
README_GEN=README.gen
doc:	
	@rm -f ${README_GEN}
	@echo "Auto created from READMEs located in ZRT project\n" > ${README_GEN}
	@find ./lib ./tests -name "README" | xargs -l1 -IFNAME sed 's@{DOCPATH}@Editable README here: FNAME@' FNAME >> ${README_GEN}
	@chmod 0444 ${README_GEN}

ARCH=x86_64-nacl
INCLUDE_DIR=$(ZVM_DESTDIR)$(ZVM_PREFIX)/${ARCH}/include
LIB_DIR=$(ZVM_DESTDIR)$(ZVM_PREFIX)/${ARCH}/lib
install:
	install -m 0644 lib/libzrt.a $(ZVM_DESTDIR)$(ZVM_PREFIX)/${ARCH}/lib
	install -m 0644 lib/libmapreduce.a $(ZVM_DESTDIR)$(ZVM_PREFIX)/${ARCH}/lib
	install -m 0644 lib/libnetworking.a $(ZVM_DESTDIR)$(ZVM_PREFIX)/${ARCH}/lib
	install -m 0644 lib/libfs.a $(ZVM_DESTDIR)$(ZVM_PREFIX)/${ARCH}/lib
	install -d $(INCLUDE_DIR)/sqlite3 $(INCLUDE_DIR)/lua $(INCLUDE_DIR)/helpers \
		$(INCLUDE_DIR)/networking $(INCLUDE_DIR)/mapreduce $(LIB_DIR)
	install -m 0644 libports/sqlite3/vfs_channel.h $(INCLUDE_DIR)/sqlite3
	install -m 0644 libports/sqlite3/sqlite3.h $(INCLUDE_DIR)/sqlite3
	install -m 0644 libports/sqlite3/sqlite3ext.h $(INCLUDE_DIR)/sqlite3
	install -m 0644 lib/lua/lauxlib.h $(INCLUDE_DIR)/lua
	install -m 0644 lib/lua/lualib.h $(INCLUDE_DIR)/lua
	install -m 0644 lib/lua/lua.h $(INCLUDE_DIR)/lua
	install -m 0644 lib/lua/luaconf.h $(INCLUDE_DIR)/lua
	install -m 0644 lib/networking/channels_conf.h $(INCLUDE_DIR)/networking
	install -m 0644 lib/networking/channels_conf_reader.h $(INCLUDE_DIR)/networking
	install -m 0644 lib/networking/eachtoother_comm.h $(INCLUDE_DIR)/networking
	install -m 0644 lib/mapreduce/map_reduce_lib.h $(INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/buffer.h $(INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/buffer.inl $(INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/buffered_io.h $(INCLUDE_DIR)/mapreduce
	install -m 0644 lib/helpers/dyn_array.h $(INCLUDE_DIR)/helpers
	install -m 0644 lib/liblua.a $(LIB_DIR)
	install -m 0644 lib/libgtest.a $(LIB_DIR)
	install -m 0644 lib/libtar.a $(LIB_DIR)
	install -m 0644 lib/libsqlite3.a $(LIB_DIR)
	install -m 0755 zvsh $(ZVM_DESTDIR)$(ZVM_PREFIX)/bin
	sed -i 's#$$ZEROVM_ROOT#$(ZVM_PREFIX)/bin#' $(ZVM_DESTDIR)$(ZVM_PREFIX)/bin/zvsh

.PHONY: install

