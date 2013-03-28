include $(ZRT_ROOT)/Makefile.env

NAME=tarimage

all: $(NAME).o
	$(CC) -o $(NAME).nexe  $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template | \
	 sed s@{MOUNTS_PATH}@${ZRT_ROOT}/mounts/@g > $(NAME).manifest
	@sed s@{NEXE_PATH}@${CURDIR}/$(NAME).nexe@g debug.template  > $(NAME).scp
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).o $(NAME).manifest *.log *.data
