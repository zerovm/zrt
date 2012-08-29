NAME=environment_test
PNACL_TOOL=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/gtest -I$(ZRT_ROOT)

all: $(NAME).nexe gtest_$(NAME).nexe run1 run2

$(NAME).nexe:
	$(PNACL_TOOL)/bin/x86_64-nacl-g++ -c -o $(NAME).o -Wall -static $(INCLUDE_FLAGS) $(DEBUG) -Wno-long-long -O2 -m64 ${NAME}.cc
	$(PNACL_TOOL)/bin/x86_64-nacl-g++ -o $(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o -L$(ZRT_ROOT)/lib -lzrt   

gtest_$(NAME).nexe:
	$(PNACL_TOOL)/bin/x86_64-nacl-g++ -c -o gtest$(NAME).o -Wall -static $(INCLUDE_FLAGS) $(DEBUG) -DGTEST -Wno-long-long -O2 -m64 ${NAME}.cc
	$(PNACL_TOOL)/bin/x86_64-nacl-g++ -o gtest_$(NAME).nexe -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) gtest$(NAME).o -L$(ZRT_ROOT)/lib -L$(ZRT_ROOT)/gtest -lzrt -lgtest   

run1:
	sed s@{ABS_PATH}@${CURDIR}/@g ${NAME}.manifest.template > ${NAME}.manifest
	$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -v2
	
run2:
	sed s@{ABS_PATH}@${CURDIR}/@g gtest_${NAME}.manifest.template > gtest_${NAME}.manifest
	$(ZEROVM_ROOT)/zerovm -Mgtest_$(NAME).manifest -v2
	
clean:
	rm -f *.nexe *.o *.log *.result *.std* *.manifest

