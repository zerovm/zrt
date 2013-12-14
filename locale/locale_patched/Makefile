NAME=locale_patcher

all: build
	sh add_all_patched_zrt_locales.sh


build: locale_patcher.o
	$(CC) -o $(NAME) $(NAME).o


clean:
	@rm $(NAME) $(NAME).o -fv
	@rm lib -rfv