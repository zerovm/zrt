include $(ZRT_ROOT)/Makefile.env
NAME=file_stat

all: $(NAME).nexe run

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)

run: $(NAME).nexe
	@echo some data > generated.data
	@echo some data > cdr.data
	@echo some data > randomread.data
	@sed s@{ABS_PATH}@${CURDIR}/@g ${NAME}.manifest.template > ${NAME}.manifest
	$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -v2

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data
