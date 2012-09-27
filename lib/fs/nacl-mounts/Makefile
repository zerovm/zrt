PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CXX=$(PNACL_TOOL)/bin/x86_64-nacl-g++
CC=$(PNACL_TOOL)/bin/x86_64-nacl-gcc

#====================== taken from nacl-mounts
# Add root of nacl-mounts to the include path
CPPFLAGS += -I.
CPPFLAGS += -I..
CPPFLAGS += -I../..
CPPFLAGS += -g -DDEBUG

UTIL_SOURCES = $(addprefix util/, Path.cc )
MEM_SOURCES = $(addprefix memory/, MemMount.cc MemNode.cc)

SOURCES = $(UTIL_SOURCES) $(BASE_SOURCES) $(MEM_SOURCES) 
OBJECTS = $(SOURCES:.cc=.o)
#======================

all: libfs.a

libfs.a: $(OBJECTS)
	@$(AR) rc $@ $(OBJECTS) 

clean:
	@find -name "*.o" | xargs rm -f *.a *.o 

