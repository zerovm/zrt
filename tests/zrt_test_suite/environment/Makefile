NAME=environment_test
PNACL_TOOL=$(shell ../../../setupenv.sh)
ZRT_ROOT=../../..
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
	@$(ZRT_ROOT)/zvm/zerovm -M$(NAME).manifest -v2
	
run2:
	@$(ZRT_ROOT)/zvm/zerovm -Mgtest_$(NAME).manifest -v2
	
clean:
	rm -f *.nexe *.o *.log *.result *.std*

