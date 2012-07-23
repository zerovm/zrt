#prerequisites: Google Native Client
PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
#ported application libraries
LIBSQLITE=lib/sqlite3/libsqlite3.a
LIBLUA=lib/lua-5.2.1/src/liblua.a
LIBMAPREDUCE=lib/mapreduce/libmapreduce.a
MACROS_FLAGS=-DUSER_SIDE
INCLUDE_PATH=-I. -Ilib -Izvm


all: lib/libzrt.a

lib/libzrt.a: ${LIBSQLITE} ${LIBLUA} ${LIBMAPREDUCE} lib/syscall_manager.S lib/zrt.c zvm/zvm.c lib/syscallbacks.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c zvm/zvm.c -o zvm/zvm.o -Wall -Wno-long-long -O2 -msse4.1 -m64 ${INCLUDE_PATH}
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/syscall_manager.S -o lib/syscall_manager.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/zrt.c -o lib/zrt.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c lib/syscallbacks.c -o lib/syscallbacks.o -Wall -Wno-long-long -O2 -m64 \
	${MACROS_FLAGS} ${INCLUDE_PATH}	
	@ar rcs lib/libzrt.a lib/syscall_manager.o zvm/zvm.o lib/zrt.o lib/syscallbacks.o
	@mv ${LIBSQLITE} lib
	@mv ${LIBLUA} lib
	@mv ${LIBMAPREDUCE} lib
	 
${LIBSQLITE}:
	@PNACL_TOOL=${PNACL_TOOL} make -Clib/sqlite3
	
${LIBLUA}:
	@PNACL_TOOL=${PNACL_TOOL} make ansi -Clib/lua-5.2.1	 

${LIBMAPREDUCE}:
	@PNACL_TOOL=${PNACL_TOOL} make -Clib/mapreduce

all_samples:
	@echo Building samples
	@make -Csamples/command_line
	@make -Csamples/accessors
	@make -Csamples/disort
	@make -Csamples/file_io	
	@make -Csamples/file_stat
	@make -Csamples/hello
	@make -Csamples/lua
	@make -Csamples/malloc
	@make -Csamples/reqrep
	@make -Csamples/sort
	@make -Csamples/sort_paging
	@make -Csamples/sqlite
	@make -Csamples/strtol
	@make -Csamples/time

clean:	
	@make -Clib/sqlite3 clean
	@make -Clib/lua-5.2.1 clean
	@make -Clib/mapreduce clean
	@rm -f lib/*.o lib/*.a

echo:
	@echo "PNACL_TOOL=${PNACL_TOOL}"
		
