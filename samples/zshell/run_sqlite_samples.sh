#!/bin/bash

echo Run sqlite samples

./genmanifest.sh sqlite/scripts/select.sql log/sqlite_output.1 /sqlite/data/zvm_netw.db log/sqlite_logfile.1 > sqlite/select_sqlite.manifest 
echo -------------------------------run sqlite #1: select
rm log/sqlite_output.1 -f
echo ../../zvm/zerovm -Msqlite/select_sqlite.manifest
../../zvm/zerovm -Msqlite/select_sqlite.manifest
echo "stdout output >>>>>>>>>>"
cat log/sqlite_output.1
