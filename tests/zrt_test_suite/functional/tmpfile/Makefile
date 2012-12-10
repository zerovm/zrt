include $(ZRT_ROOT)/Makefile.env
NAME=tmpfile

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o 
	@$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)

run:
	sed s@{ABS_PATH}@${CURDIR}/@g ${NAME}.manifest.template | \
	sed s@{ZRT_MOUNTS_PATH}@${ZRT_ROOT}/mounts/@g > ${NAME}.manifest
	@$(ZEROVM) -M$(NAME).manifest -v2

clean:
	@rm -f $(NAME).nexe $(NAME).o $(NAME).manifest *.log *.data
