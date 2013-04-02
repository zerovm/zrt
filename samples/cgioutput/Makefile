include $(ZRT_ROOT)/Makefile.env
NAME=cgioutput

CFLAGS+=-g2

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g manifest.template | sed s@{NAME}@$(NAME)@g > $(NAME).manifest
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest -v4

clean:
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
