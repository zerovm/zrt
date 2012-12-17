include $(ZRT_ROOT)/Makefile.env

#path add for includes
CFLAGS+=-I.  
CFLAGS+=-O2 -std=c99 

#sqlite debugging
CFLAGS+=-DSQLITE_TEST -DSQLITE_DEBUG -DSQLITE_ENABLE_IOTRACE -DSQLITE_DEBUG_OS_TRACE

#set required macroses
CFLAGS+=-DSQLITE_THREADSAFE=0
CFLAGS+=-DSQLITE_OMIT_LOAD_EXTENSION
#CFLAGS+=-DSQLITE_IGNORE_FLOCK_LOCK_ERRORS

libsqlite3.a: sqlite3.o vfs_channel.o
	@$(AR) rcs libsqlite3.a sqlite3.o vfs_channel.o

clean:
	@rm -f libsqlite3.a vfs_channel.o sqlite3.o 
