include $(ZRT_ROOT)/Makefile.env

#====================== taken from nacl-mounts
# Add root of nacl-mounts to the include path
CPPFLAGS += -I.
CPPFLAGS += -I..
CPPFLAGS += -I../..
CPPFLAGS += -g -DDEBUG

UTIL_SOURCES = $(addprefix $(CURDIR)/util/, Path.cc )
MEM_SOURCES = $(addprefix $(CURDIR)/memory/, MemMount.cc MemNode.cc)

SOURCES = $(UTIL_SOURCES) $(MEM_SOURCES) 
OBJECTS = $(SOURCES:.cc=.o)
#======================

all: libfs.a

#use macros BASEFILE__ if no need full srcpath in log debug file
%.o: %.cc
	$(CXX) $(CPPFLAGS) -DBASEFILE__=\"$(notdir $<)\" $< -o $@

libfs.a: $(OBJECTS)
	@$(AR) rc $@ $(OBJECTS) 

clean:
	@find -name "*.o" | xargs rm -f *.a *.o 

