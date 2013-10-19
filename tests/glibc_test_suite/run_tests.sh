#!/bin/bash

#create tar image to be injected into glibc fs
CPWD=`pwd`
cd singles/mounts/glibc-fs
rm -f ../tmp_dir.tar
tar -cf ../tmp_dir.tar tmp
cd $CPWD

#run tests
cd singles
make -j4
cd ..
echo --------------------------------------------
echo -n "Tests that were not run in section TODO: "
find ./todo -name "*.c" | wc -l
echo -n "Tests that were not run in section XFAIL: "
find ./singles/xfail -name "*.c" | wc -l
echo -n "Tests that were not run in section XEXCLUDED: "
find ./singles/xexcluded -name "*.c" | wc -l
