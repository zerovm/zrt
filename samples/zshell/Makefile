include $(ZRT_ROOT)/Makefile.env

CFLAGS+=-std=c99 -g -DDEBUG

#Add python include path
CFLAGS+=-I$(ZPYTHON_ROOT) -I$(ZPYTHON_ROOT)/Include

LDFLAGS+=-llua -lsqlite3 

LDFLAGS+=-L$(ZPYTHON_ROOT) -lpython3.2m -lm -ldl -lutil

all: createdirs clean zshell.nexe

zshell.nexe: pythonexec.o luaexec.o main.o sqlitequery.o
	$(CC) -o zshell.nexe pythonexec.o luaexec.o main.o sqlitequery.o $(LDFLAGS)  

createdirs:
	chmod u+rwx genmanifest.sh run_lua_samples.sh run_sqlite_samples.sh run_python_samples.sh python.sh \
run_python_samples.sh
	@mkdir -p log

clean: 
	@rm -f *.o
	@find -name "*.stdout" | xargs rm -f
	@find -name "*.log" | xargs rm -f
	@find -name "*.manifest" | xargs rm -f
	@rm -f zshell.nexe


