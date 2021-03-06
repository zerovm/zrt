# Generated automatically from Makefile.in by configure.
# Makefile for directory with message catalog handling in GNU NLS Utilities.
# Copyright (C) 1995 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

PACKAGE = tar
VERSION = 1.11.8

SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}
transform = s,x,x,
datadir = $(prefix)/lib
localedir = $(datadir)/locale
subdir = intl

AR = ar
CC = gcc
RANLIB = ranlib

DEFS = -DDEF_MSG_DOM_DIR=\"$(localedir)\" -DHAVE_CONFIG_H
CFLAGS = -g -O

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

SCRIPTS = linux-msg.sed po-to-tbl.sed xopen-msg.sed
HEADERS = gettext.h gettextP.h hash-string.h libgettext.h
SOURCES = bindtextdom.c dcgettext.c dgettext.c gettext.c \
finddomain.c loadmsgcat.c textdomain.c cat-compat.c
OBJECTS = $(GETTOBJS)
CATOBJS = cat-compat.o cat-id-tbl.o
GETTOBJS= bindtextdom.o dcgettext.o dgettext.o gettext.o \
finddomain.o loadmsgcat.o textdomain.o
DISTFILES = Makefile.in combine-sh $(SCRIPTS) $(HEADERS) $(SOURCES) \
stamp-cat-id cat-id-tbl.c

.SUFFIXES:
.SUFFIXES: .c .o
.c.o:
	$(COMPILE) $<

INCLUDES = -I.. -I$(srcdir)/.. -I. -I$(srcdir)/../intl \
-I../lib -I$(srcdir)/../lib

all: libintl.a


libintl.a: $(OBJECTS)
	rm -f libintl.a
	$(AR) cru libintl.a $(OBJECTS)
	$(RANLIB) libintl.a

cat-id-tbl.c: stamp-cat-id
stamp-cat-id: po-to-tbl.sed $(top_srcdir)/po/$(PACKAGE).pot
	sed -f $(srcdir)/po-to-tbl.sed $(top_srcdir)/po/$(PACKAGE).pot \
	  | sed -e "s/@PACKAGE NAME@/$(PACKAGE)/" > table.tmp
	if cmp -s table.tmp $(srcdir)/cat-id-tbl.c; then rm table.tmp; \
	else mv table.tmp $(srcdir)/cat-id-tbl.c; fi
	rm -f $(srcdir)/stamp-cat-id && echo timestamp > $(srcdir)/stamp-cat-id

install install-exec install-data: all

$(OBJECTS): ../config.h libgettext.h
bindtextdom.o finddomain.o loadmsgcat.o: gettextP.h gettext.h
cat-id-tbl.o: libgettext.h
dcgettext.o: gettextP.h gettext.h hash-string.h

tags: TAGS

TAGS: $(HEADERS) $(SOURCES)
	cd $(srcdir)/../intl && etags $(HEADERS) $(SOURCES) $(LIBHDRS) $(LIBSRCS) 

mostlyclean:
	rm -f *.aux *.cp *.cps *.dvi *.fn *.fns *.ky *.log *.pg *.toc *.tp *.vr
	rm -f *.a *.o core core.*
	rm -f *.tmp

clean: mostlyclean

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f TAGS

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist: $(DISTFILES)
	for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

Makefile: Makefile.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
