#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

./configure \
--host=x86_64-nacl \
--enable-static \
--disable-shared \
--enable-utf \
--with-sysroot=${ZRT_ROOT}
#--prefix=${ZRT_ROOT}
