#!/bin/bash

#$1 script path from current dir
SCRIPT=$(readlink -f "$0")
CURDIR=`dirname "$SCRIPT"`

#create tar image to be injected into glibc fs
tar -cf $CURDIR/mounts/tmp_dir.tar -C $CURDIR/mounts/glibc-fs tmp

#run tests
make -C $CURDIR clean prepare
make -C $CURDIR -j4

echo --------------------------------------------
echo -n "Skiped tests need to be divided to valid-invalid lists, in section XCHECK: "
find $CURDIR/xcheck -name "*.c" | wc -l
echo -n "Skiped tests in section TODO: "
find $CURDIR/todo -name "*.c" | wc -l
echo -n "Skiped tests require depedencies XFAIL: "
find $CURDIR/xfail -name "*.c" | wc -l
echo -n "Skiped tests by NACL, in section XEXCLUDED: "
find $CURDIR/xexcluded -name "*.c" | wc -l
