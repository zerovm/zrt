include $(ZRT_ROOT)/Makefile.env

NAME=lua
SRC_PATH=$(ZRT_ROOT)/lib/lua-5.2.1/src/not_for_lib
CFLAGS+=-O2 -I$(ZRT_ROOT)/lib/lua-5.2.1/src 
#it's significant to add zrt library just after lua library,
#it's needed to correctly link glibc substitute functions
LDFLAGS+=-llua -lzrt -lm

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

$(NAME).o: $(SRC_PATH)/$(NAME).c
	$(CC) -o $(NAME).o $(CFLAGS) $(SRC_PATH)/${NAME}.c 

run:
	@chmod u+rwx run_tests.sh
	@chmod u+rwx create-tar.sh
	@./run_tests.sh

clean:
	@rm -f $(NAME).nexe $(NAME).o $(NAME).log $(NAME).input $(NAME).output $(NAME).manifest *.tar *.log

