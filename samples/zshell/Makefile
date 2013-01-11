include $(ZRT_ROOT)/Makefile.env

CFLAGS+=-std=c99 -g -DDEBUG

#Add python include path
CFLAGS+=-I$(ZPYTHON_ROOT) -I$(ZPYTHON_ROOT)/Include

#it's significant to add zrt library just after lua library,
#it's needed to correctly link glibc substitute functions
LDFLAGS+=-llua -lzrt -lsqlite3 

#Add python library path and libraries
LDFLAGS+=-L$(ZPYTHON_ROOT) -lpython3.2m -lm -ldl -lutil -lzglibc

all: createdirs clean zshell.nexe

zshell.nexe: pythonexec.o luaexec.o main.o sqlitequery.o
	$(CC) -o zshell.nexe pythonexec.o luaexec.o main.o sqlitequery.o $(LDFLAGS)  

createdirs:
	chmod u+rwx genmanifest.sh run_lua_samples.sh run_sqlite_samples.sh \
run_python_samples.sh
	@mkdir -p log

clean: 
	@rm -f *.o
	@rm -f log/*.log log/*.stdout
	@rm -f sqlite/*.manifest
	@rm -f lua/*.manifest
	@rm -f zshell.nexe


