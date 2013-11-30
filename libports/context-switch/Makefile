include $(ZRT_ROOT)/Makefile.env
ARCH=x86_64-nacl
# ARCH=x86_64-linux-gnu

CC=${ARCH}-gcc
AR=${ARCH}-ar
CXX=${ARCH}-g++
RANLIB=${ARCH}-ranlib

CFLAGS=-Wno-long-long -msse4.1 -m64 -g

ZVM_PREFIX_ABSPATH = $(abspath $(ZVM_PREFIX))
ZEROVM=${ZVM_PREFIX_ABSPATH}/bin/zerovm -QPs

LDFLAGS=
CFLAGS+=

NAME=libcontext
OBJECTS=$(patsubst %.S,%.o,$(wildcard src/*.S))
OBJECTS+=$(patsubst %.c,%.o,$(wildcard src/*.c))
TESTS=$(patsubst tests/%.c,%,$(wildcard tests/tst-*.c))

all: ${NAME}.a #${TESTS}
#	@./test.sh

${NAME}.a: ${OBJECTS} 
	$(AR) rcs ${NAME}.a ${OBJECTS} 

${TESTS}: %: tests/%.o
	$(CC) -o $@ $< -lcontext -L.

clean:
	@rm ${NAME}.a tst* src/*.o tests/*.o *.conf std* *.log *.manifest >/dev/null 2>&1 || true