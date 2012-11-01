#!/bin/bash

echo Run sqlite samples

#it's db can be actualy used if zshell compiled with READ_ONLY_SQL define 
#in another case will used db loaded from tar image
READ_ONLY_DB=sqlite/data/zvm_netw.db

#select
./genmanifest.sh sqlite/scripts/select.sql log/sqlite1.stdout ${READ_ONLY_DB} log/sqlite1.stderr.log > sqlite/select_sqlite.manifest 
echo -------------------------------run sqlite #1: select
rm log/sqlite1.stdout -f
echo setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/select_sqlite.manifest
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/select_sqlite.manifest
echo "stdout output >>>>>>>>>>"
cat log/sqlite1.stdout

#create new db, insert data and select
./genmanifest.sh sqlite/scripts/zerovm_config.sql log/sqlite1.stdout ${READ_ONLY_DB} log/sqlite2.stderr.log > sqlite/create_sqlite.manifest 
echo -------------------------------run sqlite #2: create & insert & select
rm log/sqlite2.stdout -f
echo setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/create_sqlite.manifest
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msqlite/create_sqlite.manifest
echo "stdout output >>>>>>>>>>"
cat log/sqlite2.stdout
