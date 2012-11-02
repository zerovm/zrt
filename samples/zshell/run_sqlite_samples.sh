#!/bin/bash

echo Run sqlite samples

#in case if zshell compiled with READ_ONLY_SQL defined it will be used
#/dev/input channel and then database path should be mounted as input channel
READ_ONLY_INPUT_CHANNEL=sqlite/data/test_sqlite.db

#in another case if zshell compiled without READ_ONLY_SQL define then it will 
#use /dev/tarimage channel and real tar archive will be mounted
TAR_IMAGE=sqlite/data/tarfs.tar

#read database from input channel
NAME="select"
./genmanifest.sh sqlite/scripts/${NAME}.sql log/${NAME}.stdout ${READ_ONLY_INPUT_CHANNEL} log/${NAME}.stderr.log "/dev/input" > sqlite/${NAME}.manifest 
echo -------------------------------run sqlite #1: select
rm log/${NAME}.stdout -f
echo setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/${NAME}.manifest
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/${NAME}.manifest
echo "stdout output >>>>>>>>>>"
cat log/${NAME}.stdout

#read database from input channel
NAME="select_clone"
./genmanifest.sh sqlite/scripts/${NAME}.sql log/${NAME}.stdout ${TAR_IMAGE} log/${NAME}.stderr.log "/sqlite.db" > sqlite/${NAME}.manifest 
echo -------------------------------run sqlite #1: select
rm log/${NAME}.stdout -f
echo setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/${NAME}.manifest
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/${NAME}.manifest
echo "stdout output >>>>>>>>>>"
cat log/${NAME}.stdout

#create new db, insert data and select
NAME="create_insert_select"
./genmanifest.sh sqlite/scripts/${NAME}.sql log/${NAME}.stdout ${TAR_IMAGE} log/${NAME}.stderr.log "/sqlite-new.db" > sqlite/${NAME}.manifest 
echo -------------------------------run sqlite ${NAME}
rm log/${NAME}.stdout -f
echo setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/${NAME}.manifest
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/${NAME}.manifest
echo "stdout output >>>>>>>>>>"
cat log/${NAME}.stdout
