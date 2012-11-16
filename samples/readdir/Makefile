include $(ZRT_ROOT)/Makefile.env
NAME=readdir

all: $(NAME).nexe run 

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@$(BINPATCH) $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

run:
	chmod u+rwx run_tests.sh 
	./run_tests.sh

clean:
	@rm -f some.input
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
