NAME=net
NACL_TOOLCHAIN_PATH=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
CC=$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc
INCLUDE_FLAGS=-I $(ZRT_ROOT)/lib

all: $(NAME).o
	$(CC) -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o ${INCLUDE_FLAGS} -L$(ZRT_ROOT)/lib -lzrt
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)1.template > $(NAME)1.manifest
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)2.template > $(NAME)2.manifest
	@$(ZEROVM_ROOT)/zerovm -M$(NAME)1.manifest&
	@$(ZEROVM_ROOT)/zerovm -M$(NAME)2.manifest

$(NAME).o:
	$(CC) -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug* trashcan *.manifest
