PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CXX= ${PNACL_TOOL}/bin/x86_64-nacl-g++

DEBUG=-g
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm -I.
CCFLAGS=-Wall -Wno-long-long -m64 -static -DGTEST_HAS_PTHREAD=0 -DUSER_SIDE

all: libgtest.a

libgtest.a: gtest-all.o
	@ar rcs libgtest.a gtest-all.o

gtest-all.o: src/gtest-all.cc
	@$(CXX) -c -o gtest-all.o  $(INCLUDE_FLAGS) $(DEBUG) src/gtest-all.cc

clean:
	@rm -f libgtest.a *.o
