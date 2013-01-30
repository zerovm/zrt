include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-O2

NAME_1=generator.uint32_t
NAME_2=sort_uint_proper_with_args
NAME_3=tester.uint32_t

all: $(NAME_1).o $(NAME_2).o $(NAME_3).o cpuid.o
	$(CC) -o $(NAME_1).nexe $(NAME_1).o cpuid.o $(LDFLAGS)
	$(CC) -o $(NAME_2).nexe $(NAME_2).o cpuid.o $(LDFLAGS)
	$(CC) -o $(NAME_3).nexe $(NAME_3).o cpuid.o $(LDFLAGS)
	chmod u+rwx run_test.sh genmanifest.sh
	./run_test.sh

$(NAME_1).o:
$(NAME_2).o:
$(NAME_3).o:
cpuid.o:
	$(eval BASENAME:=$(basename $@))
	$(CC) -o $(BASENAME).o $(CFLAGS) $(BASENAME).c

clean:
	@rm -f *.nexe *.o *.log *.data *.manifest
