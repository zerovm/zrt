include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-I. -Iintl -Ilib
CFLAGS+=-DHAVE_CONFIG_H -D_GNU_SOURCE
CFLAGS+=-g -O 

HEADERS = lib/fnmatch.h lib/getopt.h lib/getpagesize.h lib/insremque.h lib/pathmax.h \
lib/regex.h lib/rx.h lib/safe-stat.h

SOURCES = lib/alloca.c lib/fileblocks.c lib/fnmatch.c lib/ftruncate.c lib/gmalloc.c \
lib/insremque.c lib/memset.c lib/mkdir.c lib/rename.c lib/regex.c lib/rmdir.c \
lib/rx.c lib/strstr.c lib/getdate.y lib/getopt.c lib/getopt1.clib/ stpcpy.c \
lib/xgetcwd.c lib/xmalloc.c lib/xstrdup.c lib/tarwrapper.c \
src/buffer.c src/create.c src/diffarch.c src/extract.c src/gnu.c src/list.c \
src/mangle.c src/names.c src/port.c src/rtapelib.c src/update.c

OBJECTS = lib/regex.o lib/getdate.o lib/getopt.o lib/getopt1.o lib/stpcpy.o \
lib/xgetcwd.o lib/xmalloc.o lib/xstrdup.o lib/tarwrapper.o \
src/buffer.o src/create.o src/diffarch.o src/extract.o src/gnu.o src/list.o \
src/mangle.o src/names.o src/port.o src/rtapelib.o src/update.o

all: libtar.a

libtar.a: $(OBJECTS)
	rm -f libtar.a
	$(AR) -cr libtar.a $(OBJECTS)
	$(RANLIB) libtar.a

clean:
	@rm -f *.a *.o $(OBJECTS) src/*.o intl/*.o
