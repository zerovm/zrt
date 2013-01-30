include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-O2
NAME=time
TIMESTAMP=$(shell date +%s)

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
#
	@sed 's@{ABS_PATH}@${CURDIR}/@g;'\
	's@{NAME}@$(NAME)@g;'\
	's@\(Environment = .*\)TimeStamp,[^,]*\(.*\)@\1TimeStamp, ${TIMESTAMP}\2@;'\
	  ../sample_manifest.template > $(NAME).manifest
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest
	@cat time.stdout.std

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.std *.manifest
