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
####################################ENV-nvram.c =name=FPATH, value=${TESTFILE}
ENV_FORKED-nvram.c=name=FPATH, value=${TESTFILE}
ENV-fork.c=name=FPATH, value=${TESTFILE} {BR}
ENV-fork.c+=name=new, value=1
ENV_FORKED-fork.c =$(ENV-fork.c) {BR}
ENV_FORKED-fork.c+=name=new, value=2
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
FSTAB_FORKED-nvram.c=$(FSTAB-nvram.c)
FSTAB-fork.c  =channel=/dev/mount/import.tar, mountpoint=/, access=ro, removable=no {BR}
FSTAB-fork.c +=channel=/dev/mount/import.tar, mountpoint=/test, access=ro, removable=no {BR}
#for forked session use new nvram config and do remount file, using updated removable field
FSTAB_FORKED-fork.c  =channel=/dev/mount/import.tar, mountpoint=/, access=ro, removable=yes {BR}
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


include $(ZRT_ROOT)/tests/Makefile.testengine
