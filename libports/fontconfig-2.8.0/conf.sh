#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

export CFLAGS="-I${PREFIX}/include"
export CPPFLAGS="-I${PREFIX}/include"
export LDFLAGS="-L${PREFIX}/lib"
export LIBXML2_CFLAGS="-I${PREFIX}/include/libxml2"
export LIBXML2_LIBS="-L${PREFIX}/lib -lxml2"

./configure \
--host=x86_64-nacl \
--enable-static \
--disable-shared \
--with-arch=x86_64 \
--with-freetype-config=${PREFIX}/bin/freetype-config \
--prefix=${PREFIX}
