#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

export CC="x86_64-nacl-gcc"
#export CC_FOR_BUILD="x86_64-nacl-gcc"
export CXX="x86_64-nacl-g++"
export AR="x86_64-nacl-ar"
export LD="x86_64-nacl-ld"
export RANLIB="x86_64-nacl-ranlib"
#export LD_LIBRARY_PATH=${ZVM_TOOLCHAIN}"/lib"
export CFLAGS="-I${ZRT_ROOT}/include"
export CPPFLAGS="-I${ZRT_ROOT}/include"
export LDFLAGS="-static -Wl,-s,-T${ZVM_SDK_ROOT}/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static,\
${ZRT_ROOT}/lib/zrt.o,-L${ZRT_ROOT}/lib,-lzrt,-lfs,-lstdc++"
export LIBC=""
export LIBXML2_CFLAGS="-I${ZRT_ROOT}/include/libxml2"
export LIBXML2_LIBS="-L${ZRT_ROOT}/lib -lxml2"

./configure \
--host=x86_64-nacl \
--enable-static \
--disable-shared \
--with-arch=x86_64 \
--with-freetype-config=../../bin/freetype-config \
--prefix=${ZRT_ROOT}

sed -i '/^LDFLAGS = /d' fc-case/Makefile
sed -i '/^LDFLAGS = /d' fc-lang/Makefile
sed -i '/^LDFLAGS = /d' fc-glyphname/Makefile
sed -i '/^LDFLAGS = /d' fc-arch/Makefile
sed -i '/^LDFLAGS = /d' doc/Makefile
