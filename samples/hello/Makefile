include $(ZRT_ROOT)/Makefile.env
NAME=hello

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g hello.manifest.template > hello.manifest
	@$(BINPATCH) $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest -v4
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
