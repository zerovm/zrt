#!/bin/bash
source ${ZRT_ROOT}/run.env

echo Run sqlite samples

#open database mounted into CDR channel, issue insert request and close DB
NAME="insert_select_cdr"
DB_PATH_ON_REAL_FS=sqlite/data/${NAME}.db
DB_PATH_ON_ZRT_FS=/dev/cdr
OPEN_MODE=rw
SCRIPT_PATH_ON_REAL_FS=sqlite/scripts/${NAME}.sql
OUTPUT_FILE=log/${NAME}.stdout
LOG_FILE=log/${NAME}.stderr.log
NEW_MANIFEST=sqlite/${NAME}.manifest

MANIFEST=manifest_template/zshell-cdr.manifest.template ./genmanifest.sh \
${SCRIPT_PATH_ON_REAL_FS} \
${OUTPUT_FILE} \
${DB_PATH_ON_REAL_FS} \
${LOG_FILE} \
"${DB_PATH_ON_ZRT_FS} ${OPEN_MODE}" > ${NEW_MANIFEST}
echo -------------------------------run sqlite ${NAME}
rm ${OUTPUT_FILE} -f
echo ${SETARCH} ${ZEROVM} -M${NEW_MANIFEST}
${SETARCH} ${ZEROVM} -M${NEW_MANIFEST}
echo "stdout output >>>>>>>>>>"
cat ${OUTPUT_FILE}
ls -l ${DB_PATH_ON_REAL_FS}


