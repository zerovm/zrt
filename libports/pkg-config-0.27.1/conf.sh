#!/bin/bash

ZVM_TOOLCHAIN=${ZVM_SDK_ROOT}
export PATH=${PATH}:${ZVM_TOOLCHAIN}/bin:

./configure \
--prefix=${ZRT_ROOT}
