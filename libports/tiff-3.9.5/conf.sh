#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

CFLAGS="-I$PREFIX/include" LDFLAGS="-s -L$PREFIX/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
