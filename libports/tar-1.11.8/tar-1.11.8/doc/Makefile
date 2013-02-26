# Generated automatically from Makefile.in by configure.
# Makefile for GNU tar documentation.
# Copyright (C) 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

PACKAGE = tar
VERSION = 1.11.8

SHELL = /bin/sh
srcdir = .
subdir = doc
top_srcdir = ..

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi
TOUCH = echo timestamp >

prefix = /usr/local
infodir = $(prefix)/info

.SUFFIXES:

DISTFILES = Makefile.in tar.texinfo docwarn.texi header.texi texinfo.tex \
stamp-vti version.texi \
tar.info tar.info-1 tar.info-2 tar.info-3 tar.info-4 tar.info-5 \
tar.info-6 tar.info-7

all: tar.info

info: tar.info

tar.info: tar.texinfo docwarn.texi header.texi version.texi
	cd $(srcdir) && $(MAKEINFO) tar.texinfo

dvi: tar.dvi

tar.dvi: tar.texinfo docwarn.texi header.texi
	$(TEXI2DVI) $(srcdir)/tar.texinfo

version.texi: stamp-vti
stamp-vti: tar.texinfo ../configure.in
	echo "@set EDITION $(VERSION)" > version.tmp
	echo "@set UPDATED `date -r $(srcdir)/tar.texinfo '+%B %Y'`" \
		>> version.tmp
	echo "@set VERSION $(VERSION)" >> version.tmp
	if cmp -s version.tmp $(srcdir)/version.texi; then rm version.tmp; \
	else mv version.tmp $(srcdir)/version.texi; fi
	$(TOUCH) $(srcdir)/stamp-vti

header.texi: ../src/tar.h
	sed -n '/Standard Archive/,/End of Standard/p' $(srcdir)/../src/tar.h \
	  | expand | sed 's/\([{}]\)/@\1/g' > $(srcdir)/header.texi

install: all
	$(top_srcdir)/mkinstalldirs $(infodir)
	cd $(srcdir); for file in tar.info*; do \
	  $(INSTALL_DATA) $$file $(infodir)/$$file; \
	done

uninstall:
	rm -f $(infodir)/tar.info*

mostlyclean:
	rm -f *.aux *.cp *.cps *.dvi *.fn *.fns *.ky *.log *.pg *.toc *.tp *.vr
	rm -f *.tmp

clean: mostlyclean

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
	@echo "This command is intended only for maintainers to use;"
	@echo "rebuilding the deleted files may require special tools."
	rm -f stamp-vti version.texi tar.info*

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist shar: $(DISTFILES)
	for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

Makefile: Makefile.in ../config.status
	cd .. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
