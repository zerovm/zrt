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
#in this section specify command line arguments which should be passed 
#as cmd line parameters for nexe, it should be specified in format:
#CMDLINE-xxxxxxxxx=args=cmd line argument listed here
#where xxxxxxxxx is the source file name waiting cmd line arguments
#examples:
CMDLINE-command_line.c=args=command_line.nexe argument_number_1 argument_number_2 the_last_argument
#####################################################################

#####################################################################
#in this section describe environment variables which should be set in manifest environment
#ENV-xxxxxxxx=, envname, envvalue, envname2, envvalue2
#xxxxxxx - source file name, just after '=' should be passed environment variable names
#and their value divided by comma ',', and not that just after '=' is expected comma ','
#examples: 
#ENV-environment.c=name=SafeWords, value=klaato_verada_nikto {BR} name=TTT, value=BBB
#trying to add env variable, escaping it twice because Makefile unescaping it
#waiting is env variable:  usable=1,2,3/"3,2,1"
ENV-environment.c=name=SafeWords, value=klaato_verada_nikto
#####################################################################

#####################################################################
#mapping channels, set mode for zrt channel stat: pipe, chr, file
MAPPING-lstat-stat-mapping.c=channel=/dev/stdin, mode=pipe {BR}channel=/dev/stdout, mode=char {BR}channel=/dev/stderr, mode=file
#####################################################################

#####################################################################
#in this section describe channels type for all channels in manifest which have 
#separated channels of unspecified type:
#{CHANTYPE1} {CHANTYPE2} {CHANTYPE3} possible channel types: [0, 1, 2, 3]
#this option is allow create manifest with specified channel type
#examples: 
CHANTYPE2-bigfile.c=2
CHANTYPE1-sequential.c=0
CHANTYPE2-sequential.c=0
CHANTYPE3-sequential.c=1
#####################################################################

include $(ZRT_ROOT)/tests/Makefile.testengine
