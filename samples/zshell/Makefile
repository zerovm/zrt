GTEST_CFLAGS=-DGTEST_HAS_PTHREAD=0
DEBUG=-g -DDEBUG
PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc
NACL_GCC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I$(ZRT_ROOT)/lib/lua-5.2.1/src -I$(ZRT_ROOT)/lib/sqlite3 -I.
CFLAGS=-Wall -Wno-long-long -m64 -std=c99

#ZSHELL is supported mode working without filesytem available
#in this case Database can be opened in READ_ONLY mode
#and only native read queries are available
CFLAGS+=-DREAD_ONLY_SQL

all: createdirs clean zshell.nexe

zshell.nexe: obj/luaexec.o obj/main.o obj/sqlitequery.o
	$(NACL_GCC) -o zshell.nexe  -s -static -T $(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 obj/luaexec.o obj/main.o obj/sqlitequery.o \
	-Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt -llua -lm -lsqlite3 -lfs -lstdc++ $(DEBUG)  
	binpatch zshell.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

obj/main.o: main.c
	$(NACL_GCC) -c -o obj/main.o     $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) main.c -DUSER_SIDE -D_GNU_SOURCE

obj/luaexec.o: luaexec.c
	$(NACL_GCC) -c -o obj/luaexec.o  $(INCLUDE_FLAGS) ${CFLAGS} $(DEBUG) luaexec.c -DUSER_SIDE

obj/sqlitequery.o: sqlitequery.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o obj/sqlitequery.o \
	$(INCLUDE_FLAGS) $(DEBUG) $(CFLAGS) -c sqlitequery.c 

createdirs:
	chmod u+rwx genmanifest.sh run_lua_samples.sh run_sqlite_samples.sh
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log log/*.stdout
	@rm -f sqlite/*.manifest
	@rm -f lua/*.manifest
	@rm -f zshell.nexe zshell_gcc


