include $(ZRT_ROOT)/Makefile.env
NAME=localedef

#mandatory parameters expected
#examples are
#LANG_TERRITORY=hr_HR
#CHARMAP=ISO-8859-2
LANG_TERRITORY=de_DE
CHARMAP=ISO-8859-1


all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template | \
sed s@{LANG_TERRITORY}@${LANG_TERRITORY}@g | \
sed s@{CHARMAP}@${CHARMAP}@g > $(NAME).manifest
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest -v2
	cat stdout.data
	cat stderr.data

archive:
	@sed s@{ABS_PATH}@${CURDIR}/@g $(NAME)-archive.manifest.template | \
sed s@{LANG_TERRITORY}@${LANG_TERRITORY}@g | \
sed s@{CHARMAP}@${CHARMAP}@g > $(NAME)-archive.manifest
	$(SETARCH) $(ZEROVM) -M$(NAME)-archive.manifest -v2
	cat stdout.data
	cat stderr.data

clean:
	@rm -f $(NAME).nexe $(NAME)-archive.manifest $(NAME).o *.log *.data
