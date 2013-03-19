#!/bin/bash
ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

export PKG_CONFIG="${PREFIX}/bin/pkg-config"
export CFLAGS="-I${PREFIX}/include"
export CPPFLAGS="-I${PREFIX}/include"
export LDFLAGS="-L${PREFIX}/lib"
export LIBS="-lz"

./configure \
--host=x86_64-nacl \
--enable-static \
--prefix=${ZRT_ROOT}
