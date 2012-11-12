include $(ZRT_ROOT)/Makefile.env

NAME=tarimage

all: $(NAME).o
	$(CC) -o $(NAME).nexe  $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template > $(NAME).manifest
	@$(BINPATCH) $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).o $(NAME).manifest *.log *.data
