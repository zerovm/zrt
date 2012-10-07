#CFLAGS+=-DHAS_LCHMOD
#CFLAGS+=-DHAS_CHFLAGS
#CFLAGS+=-DHAS_LCHFLAGS
#CFLAGS+=-DHAS_TRUNCATE64
#CFLAGS+=-DHAS_STAT64
CFLAGS+=-I$(ZRT_ROOT)/lib 

PNACL_TOOL=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc
CC = $(PNACL_TOOL)/bin/x86_64-nacl-gcc
NAME=fstest


all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(NAME).o -L$(ZRT_ROOT)/lib -lzrt -lfs -lstdc++

run: 
	sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template | sed s@{NAME}@$(NAME)@g > $(NAME).manifest
	chmod u+rwx fstest kill_fstest.sh run_test.sh run_zrtfs_tests.sh
	mkdir -p data
	binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

clean:
	@rm -f $(NAME).nexe $(NAME).o data/*.output data/*.log data/*.data *.manifest
