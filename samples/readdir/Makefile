include $(ZRT_ROOT)/Makefile.env
TIMESTAMP=1349019070
NAME=readdir
NAME2=getdents

all: $(NAME).nexe $(NAME2).nexe run 

$(NAME).nexe: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@sed 's@{ABS_PATH}@${CURDIR}/@g;'\
	's@{NAME}@$(NAME)@g;'\
	's@\(Environment = .*\)TimeStamp,[^,]*\(.*\)@\1TimeStamp, ${TIMESTAMP}\2@;'\
	  ../sample_manifest.template > $(NAME).manifest
	@sed s@{NEXE_PATH}@${CURDIR}/$(NAME).nexe@g debug.template  > $(NAME).scp

$(NAME2).nexe: $(NAME2).o
	$(CC) -o $(NAME2).nexe $(NAME2).o $(LDFLAGS)
	@sed 's@{ABS_PATH}@${CURDIR}/@g;'\
	's@{NAME}@$(NAME2)@g;'\
	's@\(Environment = .*\)TimeStamp,[^,]*\(.*\)@\1TimeStamp, ${TIMESTAMP}\2@;'\
	  ../sample_manifest.template > $(NAME2).manifest
	@sed s@{NEXE_PATH}@${CURDIR}/$(NAME2).nexe@g debug.template  > $(NAME2).scp

run:
	$(ZEROVM) -M$(NAME).manifest
	cat $(NAME).stdout.std
	$(ZEROVM) -M$(NAME2).manifest
	cat $(NAME2).stdout.std

clean:
	@rm -f some.input
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
	@rm -f $(NAME2).nexe $(NAME2).manifest $(NAME2).o
