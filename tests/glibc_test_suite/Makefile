include ${ZRT_ROOT}/Makefile.env
ifndef TESTS_ROOT
TESTS_ROOT=src
endif

CFLAGS+= -Iapi -Isrc

#For rt tests
LDFLAGS+= -lrt
#For dlopen
LDFLAGS+= -ldl
#math
LDFLAGS+= -lm
#crypt
LDFLAGS+= -lcrypt

################ strings CFLAGS
CFLAGS-inl-tester.c = -fno-builtin
CFLAGS-noinl-tester.c = -fno-builtin
CFLAGS-tst-strlen.c = -fno-builtin
CFLAGS-stratcliff.c = -fno-builtin
CFLAGS-test-ffs.c = -fno-builtin
CFLAGS-tst-inlcall.c = -fno-builtin
################ libio CFLAGS
CFLAGS-tst_putwc.c = -DOBJPFX=\"/tmp/\"
################ math CFLAGS
CFLAGS-test-float.c= -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-double.c= -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-ldouble.c= -fno-inline -ffloat-store -fno-builtin
CFLAGS-test-tgmath.c= -fno-builtin
CFLAGS-test-tgmath2.c= -fno-builtin
CFLAGS-test-tgmath-ret.c= -fno-builtin
# CPPFLAGS was in origin math Makefile
CLAGS-test-ifloat.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH__ \
			 -DTEST_FAST_MATH -fno-builtin
CLAGS-test-idouble.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH__ \
			 -DTEST_FAST_MATH -fno-builtin
CLAGS-test-ildoubl.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH__ \
			  -DTEST_FAST_MATH -fno-builtin
################ time CFLAGS
CFLAGS-getdate.c = -fexceptions

# it' needed to be adopted to our test suite
# It's copied as is:
# The following is a hack since we must compile scanf1{5,7}.c without any
# GNU extension.  The latter are needed, though, when internal headers
# are used.  So made sure we see the installed headers first.
CFLAGS-scanf15.c = -I../libio -I../stdlib -I../wcsmbs -I../time -I../string \
		   -I../wctype
CFLAGS-scanf17.c = -I../libio -I../stdlib -I../wcsmbs -I../time -I../string \
		   -I../wctype

################ posix CommandLine
CMDLINE-tst-pcre.c=args=/tmp/PCRE.tests
CMDLINE-tst-boost.c=args=/tmp/BOOST.tests
CMDLINE-bug-getdate1.c=args=/tmp/bug-getdate1-fmt
CMDLINE-tst-rxspencer.c=args=--utf8 /tmp/rxspencer/tests
CMDLINE-bug-glob1.c=args=/tmp

################ libio Environment 
#LOCPATH is set to /lib/locale only for demonstration of setting env var
#by glibc test suite, in anyway envvar was set into default path for localedata 
#that uses glibc, so another localisation tests will work withouth LOCPATH env
ENV-tst-swscanf.c=name=LOCPATH, value=/lib/locale

############### time Environment
ENV-tst-getdate.c=name=DATEMSK, value=/tmp/datemsk {BR}name=TZDIR, value=/tmp/getdate-data
ENV-tst-widetext.c=name=LOCPATH, value=$(common-objpfx)localedata {BR}name=LANGUAGE, value=C
ENV-tst-putenv.c=name=SOMETHING_NOBODY_USES, value=

include $(ZRT_ROOT)/tests/Makefile.testengine

prepare2:
	tar -cf tmp_dir.tar -C singles/mounts/glibc-fs tmp