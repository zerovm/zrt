include $(ZRT_ROOT)/Makefile.env
NAME=hello

CFLAGS+=-g2

all: $(NAME).o
	$(CC) -o $(NAME).nexe $(NAME).o $(LDFLAGS)
	@sed s@{ABS_PATH}@${CURDIR}/@g hello.manifest.template > hello.manifest
	$(SETARCH) $(ZEROVM) -M$(NAME).manifest -v4
	cat stdout.data

implicitly: clean $(NAME).o
	sed s@{ABS_PATH}@${CURDIR}/@g $(NAME).manifest.template > $(NAME).manifest
	$(CC) -m64 -nostdlib -nostartfiles -o $(NAME).nexe $(NAME).o  -s -static -T \
	${NACL_TOOLCHAIN}/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 --static-libgcc  -Wl,-z,relro -Wl,--hash-style=both $(NACL_TOOLCHAIN)/x86_64-nacl/lib/crt1.o $(NACL_TOOLCHAIN)/x86_64-nacl/lib/crti.o `$(CC) -m64 --print-file-name=crtbegin.o` $(NACL_TOOLCHAIN)/x86_64-nacl/lib/libc.a -lgcc -Wl,--as-needed `$(CC) -m64 --print-file-name=crtend.o` $(NACL_TOOLCHAIN)/x86_64-nacl/lib/crtn.o -L/home/yaroslav/git/zrt/lib -lzrt

clean:
	@rm -f $(NAME).nexe $(NAME).manifest $(NAME).o *.log *.data
