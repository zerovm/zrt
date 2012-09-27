PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc
CC = $(PNACL_TOOL)/bin/x86_64-nacl-gcc
NAME=cdrfs
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib

all: $(NAME).o
	$(CC) -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++
	sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template > $(NAME).manifest
	$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -v4

$(NAME).o:
	$(CC) -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data
