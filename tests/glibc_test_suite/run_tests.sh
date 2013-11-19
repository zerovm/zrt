#!/bin/bash

#create tar image to be injected into glibc fs
CPWD=`pwd`
cd singles/mounts/glibc-fs
rm -f ../tmp_dir.tar
tar -cf ../tmp_dir.tar tmp
cd $CPWD

#run tests
cd singles
make clean
make -j4
cd ..
echo --------------------------------------------
echo -n "Skiped tests need to be divided to valid-invalid lists, in section XCHECK: "
find ./singles/xcheck -name "*.c" | wc -l
echo -n "Skiped tests in section TODO: "
find ./todo -name "*.c" | wc -l
echo -n "Skiped tests require depedencies XFAIL: "
find ./singles/xfail -name "*.c" | wc -l
echo -n "Skiped tests by NACL, in section XEXCLUDED: "
find ./singles/xexcluded -name "*.c" | wc -l
