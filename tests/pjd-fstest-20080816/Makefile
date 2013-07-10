include ../../Makefile.env

#CFLAGS+=-DHAS_LCHMOD
#CFLAGS+=-DHAS_CHFLAGS
#CFLAGS+=-DHAS_LCHFLAGS
#CFLAGS+=-DHAS_TRUNCATE64
#CFLAGS+=-DHAS_STAT64

NAME=fstest
NAME_TOOL=redirect_pipe/redirect_pipe


all: $(NAME).nexe $(NAME_TOOL) run

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o

$(NAME_TOOL): $(NAME_TOOL).o
	gcc -o $(NAME_TOOL) $(NAME_TOOL).o

$(NAME_TOOL).o:
	gcc -c -o $(NAME_TOOL).o -g $(NAME_TOOL).c 

run: 
	sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template | sed s@{NAME}@$(NAME)@g > $(NAME).manifest
	chmod u+rwx fstest kill_fstest.sh run_test.sh run_zrtfs_tests.sh
	mkdir -p data

clean:
	@rm -f $(NAME).nexe $(NAME_TOOL) rm *.o data/*.output data/*.log data/*.data *.manifest
