make $@
x86_64-nacl-gcc -std=gnu99 -I/home/kit/git/zrt/include -static -Wl,-s -Wl,-T/home/kit/zvm-sdk/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -Wl,/home/kit/git/zrt/lib/zrt.o -Wl,-L/home/kit/git/zrt/lib -Wl,-lzrt -Wl,-lfs -Wl,-lstdc++ -o djpeg djpeg.o wrppm.o wrgif.o wrtarga.o wrrle.o wrbmp.o rdcolmap.o cdjpeg.o ./.libs/libjpeg.a
