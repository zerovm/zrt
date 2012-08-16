PNACL_TOOL=$(shell ./setupenv.sh)

#ported application libraries
LIBSQLITE=lib/sqlite3/libsqlite3.a
LIBLUA=lib/lua-5.2.1/src/liblua.a
LIBMAPREDUCE=lib/mapreduce/libmapreduce.a
LIBNETWORKING=lib/networking/libnetworking.a
LIBGTEST=gtest/libgtest.a
MACROS_FLAGS=-DUSER_SIDE -DDEBUG
INCLUDE_PATH=-I. -Ilib -Izvm

all: prepare lib/libzrt.a ${LIBSQLITE} ${LIBLUA} ${LIBMAPREDUCE} ${LIBGTEST} ${LIBNETWORKING} all_samples

prepare:
	chmod u+rwx setupenv.sh
	echo ${PNACL_TOOL}

lib/libzrt.a: lib/syscall_manager.S lib/zrt.c zvm/zvm.c lib/zrtsyscalls.c lib/zrtreaddir.c 
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c zvm/zvm.c -o zvm/zvm.o -Wall -Wno-long-long -O2 -msse4.1 -m64 ${INCLUDE_PATH}
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/syscall_manager.S -o lib/syscall_manager.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrtreaddir.c -o lib/zrtreaddir.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrt.c -o lib/zrt.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrtsyscalls.c -o lib/zrtsyscalls.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}	
	@ar rcs lib/libzrt.a lib/syscall_manager.o zvm/zvm.o lib/zrt.o lib/zrtsyscalls.o lib/zrtreaddir.o

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

all_samples:
	@echo Building samples
	make -Csamples/command_line
	make -Csamples/disort
	make -Csamples/environment	
	make -Csamples/file_stat
	make -Csamples/hello
	make -Csamples/manifest_test
	make -Csamples/net
	make -Csamples/readdir
	make -Csamples/reqrep
	make -Csamples/sort_paging
	make -Csamples/time
	make -Csamples/wordcount
	make -Csamples/zshell
	
clean_samples:
	@make -Csamples/command_line clean
	@make -Csamples/disort clean
	@make -Csamples/environment	clean
	@make -Csamples/file_stat clean
	@make -Csamples/hello clean
	@make -Csamples/manifest_test clean
	@make -Csamples/net clean
	@make -Csamples/readdir clean
	@make -Csamples/reqrep clean
	@make -Csamples/sort_paging clean
	@make -Csamples/time clean
	@make -Csamples/wordcount clean
	@make -Csamples/zshell clean
	
clean: clean_samples
	@make -Clib/sqlite3 clean
	@make -Clib/lua-5.2.1 clean
	@make -Clib/mapreduce clean
	@make -Clib/networking clean
	@make -Cgtest clean
	@rm -f lib/*.o lib/*.a

echo:
	@echo "PNACL_TOOL=${PNACL_TOOL}"
		
