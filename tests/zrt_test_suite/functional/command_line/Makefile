include $(ZRT_ROOT)/Makefile.env
NAME=command_line

all: $(NAME).o
	@$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	chmod u+rwx genmanifest.sh 
	./genmanifest.sh
	@$(ZEROVM_ROOT)/zerovm -M$(NAME).manifest -v2
	cat stdout.data

clean:
	@rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
