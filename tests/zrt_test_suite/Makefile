include $(ZRT_ROOT)/Makefile.env
ifndef TESTS_ROOT
TESTS_ROOT=autotests
endif

MANIFEST=manifest_zrttests
NVRAM=nvram_zrttests

#####################################################################
#in this section describe compiler flags by defining makefile variable 
#CFLAGS-xxxxxxxx= gcc flags listed here
#where xxxxxxxx is name of source file which should be compiled with additional flags
#examples: 
#CFLAGS-inl-tester.c = -fno-builtin
#CLAGS-test-ifloat.c= -U__LIBC_INTERNAL_MATH_INLINES -D__FAST_MATH_
#####################################################################

#####################################################################
#generate nvram file
#in this section specify command line arguments which should be passed 
#as cmd line parameters for nexe, it should be specified in format:
#CMDLINE-xxxxxxxxx=args=cmd line argument listed here
#where xxxxxxxxx is the source file name waiting cmd line arguments
#examples:
CMDLINE-command_line.c=args=command_line.nexe argument_number_1 argument_number_2 the_last_argument
CMDLINE-nvram.c=args=nvram.nexe
#####################################################################

#####################################################################
#generate nvram file
#in this section describe environment variables which should be set in manifest environment
#ENV-xxxxxxxx=, envname, envvalue, envname2, envvalue2
#xxxxxxx - source file name, just after '=' should be passed environment variable names
#and their value divided by comma ',', and not that just after '=' is expected comma ','
#examples: 
#ENV-environment.c=name=SafeWords, value=klaato_verada_nikto {BR} name=TTT, value=BBB
#trying to add env variable, escaping it twice because Makefile unescaping it
#waiting is env variable:  usable=1,2,3/"3,2,1"
ENV-environment.c=name=SafeWords, value=klaato_verada_nikto
ENV_FORKED-nvram.c=name=FPATH, value=${TESTFILE}{BR}
ENV-fork.c=name=FPATH, value=${TESTFILE} {BR}
ENV-fork.c+=name=new, value=1{BR}
ENV_FORKED-fork.c =$(ENV-fork.c) {BR}
ENV_FORKED-fork.c+=name=new, value=2{BR}
ENV-getcwd.c=name=HOME, value=/home/zvm{BR}
ENV-getpwuid.c=name=HOME, value=/home/zvm {BR}
ENV-getpwuid.c+=name=LOGNAME, value=zvm {BR}
ENV-getpwuid.c+=name=UID, value=2{BR}
#CURDIR is needed to fetch & create rootdir, to have the same root
#contents when compare expected and real readdir output at generic
#test and gcov test
ENV-readdir.c=name=CURDIR, value=$(CURDIR) {BR}
#set wrong home path to test behaviour in this cond
ENV-readdir.c+=name=HOME, value=/dev/stdin
#set wrong home path to test behaviour in this cond
ENV-lstat-stat-mapping.c=name=HOME, value=/dev/stdin/mount
ENV-fdopen-open.c=name=$(TRACE), value=1
ENV-constructor.c+=name=TESTDIR, value=/a/b/c/d
#####################################################################


#####################################################################
#generate nvram file,
#mapping channels, set mode for zrt channel stat: pipe, chr, file
MAPPING-lstat-stat-mapping.c =channel=/dev/stdin, mode=char {BR}
MAPPING-lstat-stat-mapping.c+=channel=/dev/stdout, mode=pipe {BR}
MAPPING-lstat-stat-mapping.c+=channel=/dev/stderr, mode=file
MAPPING-seek.c =channel=/dev/read-write, mode=file
MAPPING-fork.c =channel=/dev/read-write, mode=char
MAPPING_FORKED-fork.c =channel=/dev/read-write, mode=file
MAPPING-readdir.c =channel=/dev/mount2, mode=file {BR}
MAPPING-readdir.c +=channel=/dev/stdin, mode=char {BR}
MAPPING-readdir.c +=channel=/dev/null, mode=char {BR}
MAPPING-fdopen-open.c =channel=/dev/stdout, mode=char {BR}
MAPPING-fdopen-open.c +=channel=/dev/file, mode=file {BR}
#####################################################################

#####################################################################
#generate nvram file, 
#fstab section for mounting tar archive
#usng many tests for fstab for access=ro
FSTAB-nvram.c =channel=/dev/mount/import.tar, mountpoint=/warm, access=ro, removable=no {BR}
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/bad, access=ro, bad=removable {BR}
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/ok, access=ro, removable=yes {BR}
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/ok, access=ro, removable=yes {BR}
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/bad1, access=ro {BR}
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/bad2, access=re, removable=yes {BR}
FSTAB-nvram.c+=channel=/dev/mount/non_existing.tar, mountpoint=/bad3, access=ro, removable=yes {BR}
FSTAB-nvram.c+=channel=/dev/stdout, mountpoint=/bad3, access=ro, removable=no {BR}
FSTAB-nvram.c+=channel=/dev/stdin, mountpoint=/bad3, access=ro, removable=no {BR}
#import image into special directory to test conflicts for furter imports
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/test/test.1234, access=ro, removable=no {BR}
#import file test.1234 from tar image into dir where we have a dir
#with a such name, expected error during extraction of file
FSTAB-nvram.c +=channel=/dev/mount/import.tar, mountpoint=/test, access=ro, removable=no {BR}
#try to import image with mountpoint not in dir but file, import itself should fail
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/test/test.1234/test.1234, access=ro, removable=no {BR}
#try to import into path which have a file instead subdir
FSTAB-nvram.c+=channel=/dev/mount/import.tar, mountpoint=/test/test.1234/test.1234/bad, access=ro, removable=no {BR}
FSTAB_FORKED-nvram.c=$(FSTAB-nvram.c)
FSTAB-fork.c +=channel=/dev/mount/import.tar, mountpoint=/, access=ro, removable=no {BR}
FSTAB-fork.c +=channel=/dev/mount/import.tar, mountpoint=/test, access=ro, removable=no {BR}
#for forked session use new nvram config, update removable field's value
FSTAB_FORKED-fork.c  =channel=/dev/mount/import.tar, mountpoint=/, access=ro, removable=yes {BR}
FSTAB_FORKED-fork.c +=channel=/dev/mount/import.tar, mountpoint=/test, access=ro, removable=no {BR}
FSTAB-tmpfile.c+=channel=/dev/mount/non_existing.tar, mountpoint=/bad3, access=ro, removable=yes {BR}
FSTAB-tmpfile.c +=channel=/dev/stdout, mountpoint=/bad3, access=ro, removable=no {BR}
FSTAB-tmpfile.c +=channel=/dev/stdin, mountpoint=/bad3, access=ro, removable=no {BR}
#####################################################################

#####################################################################
#generate nvram file, precache section
#no PRECACHE available for forked session because logically it's can done only once
PRECACHE-nvram.c=precache=yes {BR}
#####################################################################

#####################################################################
#generate manifest file
#for test using fork set path for socket file
FORK-nvram.c+=$(shell mktemp -u)
FORK-fork.c+=$(shell mktemp -u)
#####################################################################

#####################################################################
#generate manifest file
#in this section describe channels type for all channels in manifest which have 
#separated channels of unspecified type:
#{CHANNEL_READONLY_TYPE} {CHANNEL_WRITEONLY_TYPE} {CHANNEL_READWRITE_TYPE} 
#possible channel type values: [0, 1, 2, 3]
#this option is allow create manifest with specified channel type
#examples: 
CHANNEL_WRITEONLY_TYPE-bigfile.c=2
CHANNEL_READWRITE_TYPE-seek.c=1
CHANNEL_READWRITE_TYPE-io.c=3
CHANNEL_READWRITE_TYPE-fcntl-1.c=3
#####################################################################

#####################################################################
#generate manifest file
#in this section describe specific channel file size for some tests
#this option is allow create manifest with specified channel size
#examples: 
CHANNEL_WRITEONLY_SIZE-bigfile.c=5368709120
#####################################################################

#####################################################################
#generate manifest file
#inject some data into channels except standard channels
#examples: 
CHANNEL_READONLY_CONTENT-devices.c=something something
#####################################################################


#####################################################################
#generate manifest file, set amount of memory 
#examples: 
MEMMAX-mmap.c=268435456
#####################################################################

#####################################################################
# add channels listed below into manifest file
CHANNELS-readdir.c=Channel=/dev/null, /dev/mount1, 0, 0, 999999, 999999, 0, 0{BR}
CHANNELS-readdir.c+=Channel=/dev/null, /dev/mount2, 0, 0, 999999, 999999, 0, 0{BR}
CHANNELS-fdopen-open.c=Channel=$(shell mktemp), /dev/blck, 3, 0, 999999, 999999, 999999, 99999{BR}
CHANNELS-fdopen-open.c+=Channel=$(shell mktemp), /dev/file, 3, 0, 999999, 999999, 999999, 99999{BR}
#####################################################################


include $(ZRT_ROOT)/tests/Makefile.testengine
