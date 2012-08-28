PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: libsqlite3.a

libsqlite3.a: sqlite3.o vfs_channel.o
	@ar rcs libsqlite3.a sqlite3.o vfs_channel.o

sqlite3.o:
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o sqlite3.o sqlite3.c \
	-Wall -Wno-long-long -O2 -m64 -I. -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION

vfs_channel.o:
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o vfs_channel.o vfs_channel.c \
	-Wall -Wno-long-long -O2 -m64 -I. -std=c99 
	
clean:
	@rm -f libsqlite3.a vfs_channel.o sqlite3.o 
