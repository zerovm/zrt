include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-I. -Iintl -Ilib
CFLAGS+=-DHAVE_CONFIG_H -D_GNU_SOURCE
CFLAGS+=-g -O 

HEADERS = lib/fnmatch.h lib/getopt.h lib/getpagesize.h lib/insremque.h lib/pathmax.h \
lib/safe-stat.h

OBJECTS_NO_PREFIX = lib/getdate.o lib/getopt.o lib/getopt1.o lib/stpcpy.o \
lib/xgetcwd.o lib/xmalloc.o lib/xstrdup.o lib/tarwrapper.o \
src/buffer.o src/create.o src/diffarch.o src/extract.o src/gnu.o src/list.o \
src/mangle.o src/names.o src/port.o src/rtapelib.o src/update.o

OBJECTS=$(addprefix $(CURDIR)/, $(OBJECTS_NO_PREFIX))

all: libtar.a

libtar.a: $(OBJECTS)
	rm -f libtar.a
	$(AR) -cr libtar.a $(OBJECTS)
	$(RANLIB) libtar.a

clean:
	@rm -f *.a *.o $(OBJECTS) src/*.o intl/*.o
