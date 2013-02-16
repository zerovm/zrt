include $(ZRT_ROOT)/Makefile.env

DEBUG=-g
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I.
CCFLAGS+=-DGTEST_HAS_PTHREAD=0

all: libgtest.a

libgtest.a: gtest-all.o
	@ar rcs libgtest.a gtest-all.o

gtest-all.o: src/gtest-all.cc
	@$(CXX) -c -o gtest-all.o  $(INCLUDE_FLAGS) $(DEBUG) src/gtest-all.cc

clean:
	@rm -f libgtest.a *.o
