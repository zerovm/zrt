include $(ZRT_ROOT)/Makefile.env
NAME=readdir

all: $(NAME).nexe run 

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)

run:
	chmod u+rwx run_tests.sh 
	./run_tests.sh

clean:
	@rm -f some.input
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
