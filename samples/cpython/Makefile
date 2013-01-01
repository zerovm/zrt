include $(ZRT_ROOT)/Makefile.env

NAME=cpython

all:
	@cp $(ZRT_ROOT)/../cpython_for_zrt/Python-3.2.2/python ./cpython.nexe
#	$(CC) -o $(NAME).nexe  $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template | \
	 sed s@{MOUNTS_PATH}@${ZRT_ROOT}/mounts/@g > $(NAME).manifest
#	@$(BINPATCH) $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).manifest *.log *.data
