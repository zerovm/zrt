#!/bin/bash

./configure

make python Parser/pgen

mv python hostpython

mv Parser/pgen Parser/hostpgen

make distclean

patch -p1 < Python-3.2.2-xcompile.patch

#nacl support patches
patch pyconfig.h.in < pyconfig.h.in.patch
patch Modules/python.c < modules_python.c.patch
patch Modules/_testembed.c < modules_testembed.c.patch