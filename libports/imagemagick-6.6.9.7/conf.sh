#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

export CC="x86_64-nacl-gcc"
export CXX="x86_64-nacl-g++"
export AR="x86_64-nacl-ar"
export RANLIB="x86_64-nacl-ranlib"
#export LD_LIBRARY_PATH=${ZVM_TOOLCHAIN}"/lib"
export CFLAGS="-I${ZRT_ROOT}/include -I${ZRT_ROOT}/include/libxml2"
export LDFLAGS="-static -m64 -Wl,-T${ZVM_SDK_ROOT}/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static,\
${ZRT_ROOT}/lib/zrt.o,-L${ZRT_ROOT}/lib,-lzrt,-lfs,-lstdc++"
export LIBC=""
export FREETYPE_CFLAGS="-I${ZRT_ROOT}/include/freetype2"
export FONTCONFIG_CFLAGS="-I${ZRT_ROOT}/include/fontconfig"

./configure \
--host=x86_64-nacl \
--enable-static \
--without-threads \
--without-pango \
--disable-largefile \
--prefix=${ZRT_ROOT}
