include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-O2
NAME=time

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
#
	sed s@{ABS_PATH}@${CURDIR}/@g ${ZRT_ROOT}/manifest1.template | \
	sed s@{NAME}@$(NAME)@g > $(NAME).manifest
#
	binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
#
	$(SETARCH) $(ZEROVM_ROOT)/zerovm -M$(NAME).manifest

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.std *.manifest
