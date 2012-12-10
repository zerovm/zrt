include $(ZRT_ROOT)/Makefile.env
NAME=bigfile
CFLAGS+=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	@$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)

run:
	chmod u+rwx genmanifest.sh
	./genmanifest.sh
#note that zerovm option -l6 is used because working with big file
	@$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -l6 -v4
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
