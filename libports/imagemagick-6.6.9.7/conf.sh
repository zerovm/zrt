#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

export CFLAGS="-I${PREFIX}/include -I${PREFIX}/include/libxml2"
export LDFLAGS="-s -L${PREFIX}/lib"
export FREETYPE_CFLAGS="-I${PREFIX}/include/freetype2"
export FREETYPE_LIBS="-L${PREFIX}/lib -lfreetype"
export FONTCONFIG_CFLAGS="-I${PREFIX}/include/fontconfig"
export FONTCONFIG_LIBS="-L${PREFIX}/lib -lfontconfig"
export PKG_CONFIG="${PREFIX}/bin/pkg-config"
export ac_cv_prog_freetype_config="${PREFIX}/bin/freetype-config"
export ac_cv_sys_file_offset_bits=64

./configure \
--host=x86_64-nacl \
--enable-static \
--enable-largefile \
--without-threads \
--with-fontconfig \
--disable-assert \
--prefix=${PREFIX}
