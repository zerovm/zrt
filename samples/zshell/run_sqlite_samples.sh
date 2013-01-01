#!/bin/bash
source ${ZRT_ROOT}/run.env

echo Run sqlite samples

#open database mounted into read-only channel
READ_ONLY_INPUT_CHANNEL=sqlite/data/test_sqlite.db

#use /dev/tarimage channel and real tar archive will be mounted
TAR_IMAGE=sqlite/data/tarfs.tar

#read database from input channel
NAME="select"
MANIFEST=manifest_template/zshell.manifest.template ./genmanifest.sh sqlite/scripts/${NAME}.sql log/${NAME}.stdout ${READ_ONLY_INPUT_CHANNEL} log/${NAME}.stderr.log "/dev/input ro" > sqlite/${NAME}.manifest 
echo -------------------------------run sqlite ${NAME}
rm log/${NAME}.stdout -f
echo ${SETARCH} ${ZEROVM} -Msqlite/${NAME}.manifest
${SETARCH} ${ZEROVM} -Msqlite/${NAME}.manifest
echo "stdout output >>>>>>>>>>"
cat log/${NAME}.stdout

#read database from input channel
NAME="select_clone"
MANIFEST=manifest_template/zshell.manifest.template ./genmanifest.sh sqlite/scripts/${NAME}.sql log/${NAME}.stdout ${TAR_IMAGE} log/${NAME}.stderr.log "/sqlite.db rw" > sqlite/${NAME}.manifest 
echo -------------------------------run sqlite ${NAME}
rm log/${NAME}.stdout -f
echo ${SETARCH} ${ZEROVM} -Msqlite/${NAME}.manifest
${SETARCH} ${ZEROVM} -Msqlite/${NAME}.manifest
echo "stdout output >>>>>>>>>>"
cat log/${NAME}.stdout

#create new db, insert data and select
NAME="create_insert_select"
MANIFEST=manifest_template/zshell.manifest.template ./genmanifest.sh sqlite/scripts/${NAME}.sql log/${NAME}.stdout ${TAR_IMAGE} log/${NAME}.stderr.log "/sqlite-new.db rw" > sqlite/${NAME}.manifest 
echo -------------------------------run sqlite ${NAME}
rm log/${NAME}.stdout -f
echo ${SETARCH} ${ZEROVM} -Msqlite/${NAME}.manifest
${SETARCH} ${ZEROVM} -Msqlite/${NAME}.manifest
echo "stdout output >>>>>>>>>>"
cat log/${NAME}.stdout

