PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

#path add for includes
CFLAGS=-I.  
CFLAGS+=-Wall -Wno-long-long -O2 -m64 -std=c99 

#sqlite debugging
CFLAGS+=-DSQLITE_TEST -DSQLITE_DEBUG -DSQLITE_ENABLE_IOTRACE -DSQLITE_DEBUG_OS_TRACE

#set required macroses
CFLAGS+=-DSQLITE_THREADSAFE=0
CFLAGS+=-DSQLITE_OMIT_LOAD_EXTENSION
CFLAGS+=-DSQLITE_IGNORE_FLOCK_LOCK_ERRORS

CC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc
AR=$(PNACL_TOOL)/bin/x86_64-nacl-ar

all: libsqlite3.a

libsqlite3.a: sqlite3.o vfs_channel.o
	@$(AR) rcs libsqlite3.a sqlite3.o vfs_channel.o

clean:
	@rm -f libsqlite3.a vfs_channel.o sqlite3.o 
