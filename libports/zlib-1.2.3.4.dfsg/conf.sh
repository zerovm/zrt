#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:
export CC="x86_64-nacl-gcc"
#export CXX="x86_64-nacl-g++"
#export AR="x86_64-nacl-ar"
#export RANLIB="x86_64-nacl-ranlib"
#export CFLAGS="-I${ZRT_ROOT}/lib "
#export LDFLAGS="-static"
#export LDFLAGS="-static -Wl,-s,-T${ZVM_SDK_ROOT}/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static"

./configure --static
make clean all

