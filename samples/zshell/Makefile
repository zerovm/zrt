include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -g -DDEBUG
CFLAGS+=-I$(ZRT_ROOT)/lib/lua-5.2.1/src -I$(ZRT_ROOT)/lib/sqlite3

LDFLAGS+=-llua -lm -lsqlite3

#ZSHELL is supported mode working without available filesytem 
#in this case Database can be readed via READ_ONLY sequential 
#channel and only native read queries are available
CFLAGS+=-DREAD_ONLY_SQL

all: createdirs clean zshell.nexe

zshell.nexe: obj/luaexec.o obj/main.o obj/sqlitequery.o
	$(CC) -o zshell.nexe obj/luaexec.o obj/main.o obj/sqlitequery.o $(LDFLAGS)  
	binpatch zshell.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

obj/main.o: main.c
	$(CC) -c -o obj/main.o ${CFLAGS} main.c -D_GNU_SOURCE

obj/luaexec.o: luaexec.c
	$(CC) -c -o obj/luaexec.o ${CFLAGS} luaexec.c

obj/sqlitequery.o: sqlitequery.c
	@$(CC) -c -o obj/sqlitequery.o $(CFLAGS) sqlitequery.c 

createdirs:
	chmod u+rwx genmanifest.sh run_lua_samples.sh run_sqlite_samples.sh
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log log/*.stdout
	@rm -f sqlite/*.manifest
	@rm -f lua/*.manifest
	@rm -f zshell.nexe zshell_gcc


