include Makefile.env

############### libzrt.a source files to build
LIBDEP_SOURCES= lib/zrt.c

LIBZRT=lib/libzrt.a
LIBZRT_SOURCES= \
lib/zcalls/zcalls_prolog.c \
lib/zcalls/zcalls_zrt.c \
lib/libc/fcntl.c \
lib/libc/link.c \
lib/libc/unlink.c \
lib/libc/rename.c \
lib/libc/readlink.c \
lib/libc/symlink.c \
lib/libc/statvfs.c \
lib/libc/rmdir.c \
lib/libc/mkdir.c \
lib/libc/chmod.c \
lib/libc/chown.c \
lib/libc/ftruncate.c \
lib/libc/stat_realpath.c \
lib/libc/getsysstats.c \
lib/libc/fchdir.c \
lib/zrtlog.c \
lib/enum_strings.c \
lib/helpers/dyn_array.c \
lib/helpers/printf_prolog.c \
lib/helpers/conf_parser.c \
lib/helpers/path_utils.c \
lib/helpers/conf_keys.c \
lib/helpers/utils.c \
lib/helpers/buffered_io.c \
lib/helpers/bitarray.c \
lib/memory/memory_syscall_handlers.c \
lib/nvram/nvram_loader.c \
lib/nvram/observers/args_observer.c \
lib/nvram/observers/environment_observer.c \
lib/nvram/observers/fstab_observer.c \
lib/nvram/observers/settime_observer.c \
lib/nvram/observers/debug_observer.c \
lib/nvram/observers/mapping_observer.c \
lib/nvram/observers/precache_observer.c \
lib/fs/dirent_engine.c \
lib/fs/fcntl_implem.c \
lib/fs/mounts_manager.c \
lib/fs/handle_allocator.c \
lib/fs/open_file_description.c \
lib/fs/channels_array.c \
lib/fs/channels_mount.c \
lib/fs/channels_readdir.c \
lib/fs/transparent_mount.c \
lib/fs/unpack/mounts_reader.c \
lib/fs/unpack/unpack_tar.c \
lib/fs/unpack/image_engine.c \
lib/fs/unpack/parse_path.c \
lib/zrt.c \
lib/ptrace.c 

ifndef __ZRT_SO
LIBZRT_SOURCES += lib/fs/mem_mount_wraper.cc
endif

LIBDEP_OBJECTS_NO_PREFIX=$(addsuffix .o, $(basename $(LIBDEP_SOURCES) ) )
LIBDEP_OBJECTS=$(addprefix $(ZRT_ROOT)/, $(LIBDEP_OBJECTS_NO_PREFIX))
LIBZRT_OBJECTS_NO_PREFIX=$(addsuffix .o, $(basename $(LIBZRT_SOURCES) ) )
LIBZRT_OBJECTS=$(addprefix $(ZRT_ROOT)/, $(LIBZRT_OBJECTS_NO_PREFIX))

############## zrtlibs and ported libraries build
LIBS= lib/mapreduce/libmapreduce.a \
lib/networking/libnetworking.a

ifndef __ZRT_HOST
LIBPORTS= libports/gtest/libgtest.a \
libports/lua-5.2.1/liblua.a \
libports/sqlite3/libsqlite3.a \
libports/context-switch/libcontext.a
else
LIBPORTS= libports/tar-1.11.8/libtar.a
endif

#file inside dir will be built and installed by pth's make,
#so we need only subpath to enter dir by make
PTH=libports/pth-2.0.7/pthread.o

################# samples to build
TEST_SUITES=lua_test_suite glibc_test_suite

################# flags set
CFLAGS += -Werror-implicit-function-declaration
CFLAGS += -DDEBUG
CFLAGS += -DUSER_SIDE

################# "make all" Build zrt & run minimal tests set
all: build 
ifndef __ZRT_SO
all: autotests
endif

############## "make test" Run all test suites available for ZRT
check: build
	@echo ------------- RUN zrt tests ------------
#zrt tests
	@TESTS_ROOT=tests make -Ctests/zrt_test_suite clean prepare
	@TESTS_ROOT=tests make -Ctests/zrt_test_suite -j4
#glibc tests
	@sh tests/glibc_test_suite/run_tests.sh
#lua tests
	@make -Ctests/lua_test_suite
	@./kill_daemons.sh

################ "make gcov" Run tests and create document reflecting test coverage
trace: CPPFLAGS+=${TRACE_FLAGS}
trace: CFLAGS+=${TRACE_FLAGS}
trace: TEST_PARAM=trace
trace: all

################ "make gcov" Run tests and create document reflecting test coverage
gcov: LDFLAGS+=${GCOV_LDFLAGS}
gcov: CPPFLAGS+=${GCOV_FLAGS}
gcov: CFLAGS+=${GCOV_FLAGS}
gcov: build
	@echo ------------- RUN coverage tests $@ ------------
	@TESTS_ROOT=tests make -Ctests/zrt_test_suite clean prepare
	@TESTS_ROOT=tests make -Ctests/zrt_test_suite gcov
	@./kill_daemons.sh
	@mkdir $(GCOV_TEMP_FOLDER) $(GCOV_HTML_FOLDER) -p
	@tar xvf $(GCOV_DATA_TAR) -C $(GCOV_TEMP_FOLDER) > /dev/null 2>&1
	@cp $(GCOV_TEMP_FOLDER)$(ZRT_ROOT)/lib $(ZRT_ROOT) -r
	@rm $(GCOV_TEMP_FOLDER) -r -f
#prepare html document covering only sources from lib folder
	@lcov --gcov-tool=${GCOV} --directory=$(ZRT_ROOT)/lib --capture --output-file $(GCOV_HTML_FOLDER)/app.info
	@genhtml --output-directory $(GCOV_HTML_FOLDER) $(GCOV_HTML_FOLDER)/app.info
	@echo ------------- RUN coverage tests OK $@ ------------
	@echo open $(GCOV_HTML_FOLDER)/index.html

zrt_test_suite: build
#Run different groups of zrt tests sequentially
	@echo ------------- RUN zrt $@  ------------
	@TESTS_ROOT=tests make -Ctests/zrt_test_suite clean prepare
	@TESTS_ROOT=tests make -Ctests/zrt_test_suite -j4 $(TEST_PARAM)

############## "make autotests" run zrt autotests
autotests possible_slow_autotests: build
	@echo ------------- RUN zrt $@ ------------
	@TESTS_ROOT=tests/$@ make -Ctests/zrt_test_suite clean prepare
	@TESTS_ROOT=tests/$@ make  -Ctests/zrt_test_suite -j4 $(TEST_PARAM)
	@./kill_daemons.sh

#ifndef __ZRT_HOST
build: ${PTH}
#endif
build: doc  ${LIBS} ${LIBPORTS} ${LIBDEP_OBJECTS} ${LIBZRT}
ifndef __ZRT_HOST
	@make -C locale/locale_patched
endif

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

${PTH}:
ifdef __ZRT_HOST
	cd $(dir ${PTH}) && ./configure CFLAGS=-fPIC prefix=$(ZVM_PREFIX)/x86_64 \
	--enable-pthread --enable-shared=no --enable-tests=no --enable-optimize=no \
	--with-mctx-mth=mcsc \
	--with-mctx-dsp=sc \
	--with-mctx-stk=mc
	cd $(CURDIR)
endif
	@make -C$(dir $@) clean all

lua_test_suite: build
	@make -Ctests/$@

glibc_test_suite: build
	tar -cf tests/glibc_test_suite/mounts/tmp_dir.tar -C tests/glibc_test_suite/mounts/glibc-fs tmp
#run tests
	make -C tests/glibc_test_suite clean
	make -C tests/glibc_test_suite -j4

################ "make clean" Cleaning libs
LIBS_CLEAN =$(foreach smpl, ${LIBS}, $(smpl).clean)
LIBPORTS_CLEAN =$(foreach smpl, ${LIBPORTS}, $(smpl).clean)
PTH_CLEAN =$(foreach smpl, ${PTH}, $(smpl).clean)

################ "make clean" Cleaning libs, tests, samples
clean: libclean testclean gcovclean
	@rm -f lib/*.a

gcovclean:
	@rm -f -r $(GCOV_HTML_FOLDER)
ifneq ($(strip $(LCOV_EXIST)),)
	@lcov --base-directory . --directory . --zerocounters -q
endif
	@rm -f $(GCOV_DATA_TAR)
	@rm -f -r $(GCOV_TEMP_FOLDER)
	@find -name "*.gcda" | xargs rm -f
	@find -name "*.gcno" | xargs rm -f
	@find -name "*.gcov" | xargs rm -f

libclean: ${LIBS_CLEAN} testclean libportsclean pthclean
${LIBS_CLEAN}: cleandep
	@rm -f $(LIBZRT_OBJECTS)
	@rm -f $(LIBS) $(LIBZRT)
	@make -C$(dir $@) clean

pthclean: ${PTH_CLEAN}
${PTH_CLEAN}:
	@make -C$(dir $@) clean

libportsclean: ${LIBPORTS_CLEAN} 
${LIBPORTS_CLEAN}:
	@make -C$(dir $@) clean
	@rm -f $@

testclean:
ifndef __ZRT_HOST
	@make -C locale/locale_patched clean
	@make -Ctests/glibc_test_suite clean
	@make -Ctests/lua_test_suite clean
	@TESTS_ROOT=tests/autotests make -Ctests/zrt_test_suite clean
	@TESTS_ROOT=tests/possible_slow_autotests make -Ctests/zrt_test_suite clean
endif
cleandep:
	@rm -f ${LIBDEP_OBJECTS}

################ "make doc" Generate doc file concatenating all READMEs
README_GEN=README.gen
doc:
	@rm -f ${README_GEN}
	@echo "Auto created from READMEs located in ZRT project\n" > ${README_GEN}
	@find ./lib ./tests -name "README" | xargs -l1 -IFNAME sed 's@{DOCPATH}@Editable README here: FNAME@' FNAME >> ${README_GEN}
	@chmod 0444 ${README_GEN}

uninstall:
	rm -f $(INSTALL_INCLUDE_DIR)/mapreduce/buffered_io.h

install: uninstall
	@make -C$(dir ${PTH}) install
ifndef __ZRT_HOST
	install -d $(INSTALL_INCLUDE_DIR)/sqlite3 $(INSTALL_INCLUDE_DIR)/lua 
	install -m 0644 lib/liblua.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/libgtest.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/libsqlite3.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/libcontext.a $(INSTALL_LIB_DIR)
	install -m 0644 libports/sqlite3/vfs_channel.h $(INSTALL_INCLUDE_DIR)/sqlite3
	install -m 0644 libports/sqlite3/sqlite3.h $(INSTALL_INCLUDE_DIR)/sqlite3
	install -m 0644 libports/sqlite3/sqlite3ext.h $(INSTALL_INCLUDE_DIR)/sqlite3
	install -m 0644 lib/lua/lauxlib.h $(INSTALL_INCLUDE_DIR)/lua
	install -m 0644 lib/lua/lualib.h $(INSTALL_INCLUDE_DIR)/lua
	install -m 0644 lib/lua/lua.h $(INSTALL_INCLUDE_DIR)/lua
	install -m 0644 lib/lua/luaconf.h $(INSTALL_INCLUDE_DIR)/lua
endif
ifndef __ZRT_SO
	install -m 0644 lib/libfs.a $(INSTALL_LIB_DIR)
endif
	install -d $(INSTALL_INCLUDE_DIR)/networking $(INSTALL_INCLUDE_DIR)/mapreduce $(INSTALL_LIB_DIR) \
	$(INSTALL_INCLUDE_DIR)/helpers $(INSTALL_LIB_DIR)
	install -m 0644 lib/libzrt.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/libmapreduce.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/libnetworking.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/libtar.a $(INSTALL_LIB_DIR)
	install -m 0644 lib/zrtapi.h $(INSTALL_INCLUDE_DIR)
	install -m 0644 lib/networking/channels_conf.h $(INSTALL_INCLUDE_DIR)/networking
	install -m 0644 lib/networking/channels_conf_reader.h $(INSTALL_INCLUDE_DIR)/networking
	install -m 0644 lib/networking/eachtoother_comm.h $(INSTALL_INCLUDE_DIR)/networking
	install -m 0644 lib/mapreduce/map_reduce_lib.h $(INSTALL_INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/buffer.h $(INSTALL_INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/buffer.inl $(INSTALL_INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/map_reduce_datatypes.h $(INSTALL_INCLUDE_DIR)/mapreduce
	install -m 0644 lib/mapreduce/elastic_mr_item.h $(INSTALL_INCLUDE_DIR)/mapreduce
	install -m 0644 lib/helpers/dyn_array.h $(INSTALL_INCLUDE_DIR)/helpers
	install -m 0644 lib/helpers/buffered_io.h $(INSTALL_INCLUDE_DIR)/helpers

.PHONY: install

#use macros BASEFILE__ if no need full srcpath in log debug file
%.o: %.c
	$(CC) $(CFLAGS) -DBASEFILE__=\"$(notdir $<)\" $< -o $@
%.o: %.cc
	$(CXX) $(CPPFLAGS) -DBASEFILE__=\"$(notdir $<)\" $< -o $@


