PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc

#ported application libraries
LIBSQLITE=lib/sqlite3/libsqlite3.a
LIBLUA=lib/lua-5.2.1/src/liblua.a
LIBMAPREDUCE=lib/mapreduce/libmapreduce.a
LIBNETWORKING=lib/networking/libnetworking.a
LIBGTEST=gtest/libgtest.a
MACROS_FLAGS=-DUSER_SIDE -DDEBUG
INCLUDE_PATH=-I. -Ilib -I${ZEROVM_ROOT}/api 

all: prepare lib/libzrt.a ${LIBSQLITE} ${LIBLUA} ${LIBMAPREDUCE} ${LIBGTEST} ${LIBNETWORKING}

prepare:


lib/libzrt.a: lib/syscall_manager.S lib/zrt.c ${ZEROVM_ROOT}/api/zvm.c lib/zrtsyscalls.c lib/zrtreaddir.c 
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c ${ZEROVM_ROOT}/api/zvm.c -o lib/zvm.o -Wall -Wno-long-long -O2 -msse4.1 -m64 \
	${INCLUDE_PATH}
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/syscall_manager.S -o lib/syscall_manager.o
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrtreaddir.c -o lib/zrtreaddir.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrt.c -o lib/zrt.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}
	$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrtsyscalls.c -o lib/zrtsyscalls.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}	
	ar rcs lib/libzrt.a lib/syscall_manager.o lib/zvm.o lib/zrt.o lib/zrtsyscalls.o lib/zrtreaddir.o

${LIBSQLITE}:
	@make -Clib/sqlite3
	@mv -f ${LIBSQLITE} lib
	
${LIBLUA}:
	@make -Clib/lua-5.2.1
	@mv -f ${LIBLUA} lib	 

${LIBMAPREDUCE}:
	@make -Clib/mapreduce
	@mv -f ${LIBMAPREDUCE} lib

${LIBNETWORKING}:
	@PNACL_TOOL=${PNACL_TOOL} make -Clib/networking
	@mv -f ${LIBNETWORKING} lib

${LIBGTEST}:
	@PNACL_TOOL=${PNACL_TOOL} make -Cgtest

zrt_tests:
	@echo Building zrt test
	make -Ctests/zrt_test_suite/samples/bigfile
	make -Ctests/zrt_test_suite/samples/command_line
	make -Ctests/zrt_test_suite/samples/environment	
	make -Ctests/zrt_test_suite/samples/file_stat
	make -Ctests/zrt_test_suite/samples/seek
	
all_samples:
	@echo Building samples
	make -Csamples/disort
	make -Csamples/net
	make -Csamples/hello
	make -Csamples/readdir
	make -Csamples/reqrep
	make -Csamples/sort_paging
	make -Csamples/time
	make -Csamples/wordcount
	make -Csamples/zshell
	
clean_samples:
	@make -Csamples/disort clean
	@make -Csamples/net clean
	@make -Csamples/hello clean
	@make -Csamples/readdir clean
	@make -Csamples/reqrep clean
	@make -Csamples/sort_paging clean
	@make -Csamples/time clean
	@make -Csamples/wordcount clean
	@make -Csamples/zshell clean
	@make -Ctests/zrt_test_suite/samples/bigfile clean
	@make -Ctests/zrt_test_suite/samples/command_line clean
	@make -Ctests/zrt_test_suite/samples/environment	clean
	@make -Ctests/zrt_test_suite/samples/file_stat clean
	@make -Ctests/zrt_test_suite/samples/seek clean
	
clean: clean_samples
	@make -Clib/sqlite3 clean
	@make -Clib/lua-5.2.1 clean
	@make -Clib/mapreduce clean
	@make -Clib/networking clean
	@make -Cgtest clean
	@rm -f lib/*.o lib/*.a

echo:
	@echo "PNACL_TOOL=${PNACL_TOOL}"
		
