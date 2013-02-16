include $(ZRT_ROOT)/Makefile.env
NAME=hello

CFLAGS+=-g2

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g hello.manifest.template > hello.manifest
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest -v4
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
