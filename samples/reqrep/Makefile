include $(ZRT_ROOT)/Makefile.env

NAME=reqrep
CFLAGS+=-std=c99

all: createdirs reqrep.nexe run

reqrep.nexe: reqrep.o
	@$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)

run:
	chmod u+rwx genmanifest.sh test.sh test-type2.sh
	binpatch reqrep.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	./genmanifest.sh
	./test.sh	
	./test-type2.sh

createdirs:
	@mkdir -p log

clean: 
	@rm -f log/*.log
	@rm -f *.nexe
	@rm -f *.manifest


