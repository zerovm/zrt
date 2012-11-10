include $(ZRT_ROOT)/Makefile.env
NAME=seek

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o 
	@$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)

run:
	sed s@{ABS_PATH}@${CURDIR}/@g ${NAME}.manifest.template > ${NAME}.manifest
	@$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -v2

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data
