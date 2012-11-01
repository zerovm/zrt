include $(ZRT_ROOT)/Makefile.env
NAME=net

CFLAGS+=-O2

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2 > /dev/null
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)1.template > $(NAME)1.manifest
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)2.template > $(NAME)2.manifest
	@${ZRT_ROOT}/ns_start.sh 2
	@$(SETARCH) $(ZEROVM_ROOT)/zerovm -M$(NAME)1.manifest&
	@$(SETARCH) $(ZEROVM_ROOT)/zerovm -M$(NAME)2.manifest
	@${ZRT_ROOT}/ns_stop.sh

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug* trashcan *.manifest
