GTEST_CFLAGS=-DGTEST_HAS_PTHREAD=0
DEBUG=-g -DDEBUG
PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc
NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I$(ZRT_ROOT)/lib/lua-5.2.1/src -I$(ZRT_ROOT)/lib/sqlite3 -I.
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

all: createdirs clean zshell.nexe #zshell_gcc

zshell.nexe: obj/luaexec.o obj/main.o obj/sqlitequery.o
	$(NACL_GCC) -o zshell.nexe  -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 obj/luaexec.o obj/main.o obj/sqlitequery.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -llua -lm -lsqlite3 -lfs -lstdc++ $(DEBUG)  
		
obj/main.o: main.c
	$(NACL_GCC) -c -o obj/main.o     $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) main.c -DUSER_SIDE -D_GNU_SOURCE

obj/luaexec.o: luaexec.c
	$(NACL_GCC) -c -o obj/luaexec.o  $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) luaexec.c -DUSER_SIDE

obj/sqlitequery.o: sqlitequery.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o obj/sqlitequery.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 sqlitequery.c 

createdirs:
	chmod u+rwx genmanifest.sh run_lua_samples.sh run_sqlite_samples.sh
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log log/*.stdout
	@rm -f sqlite/*.manifest
	@rm -f lua/*.manifest
	@rm -f zshell.nexe zshell_gcc
	

