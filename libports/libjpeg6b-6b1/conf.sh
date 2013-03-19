#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

./configure \
--host=x86_64-nacl \
--enable-static \
--disable-shared \
--without-pic \
--prefix=${ZRT_ROOT}
