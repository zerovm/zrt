#!/bin/bash
source ${ZRT_ROOT}/run.env

PREFIX_NAME="python_"
NAME="hello"
#name provided as parameter
SCRIPT_PATH_ON_REAL_FS="$1"
OUTPUT_FILE="$1".stdout
DATA_FILE=/dev/null
LOG_FILE="$1".stderr.log
COMMAND_LINE="/dev/stdin $2 $3 $4 $5 $6 $7"
NEW_MANIFEST="$1".manifest

MANIFEST=manifest_template/zshell-python.manifest.template ./genmanifest.sh \
${SCRIPT_PATH_ON_REAL_FS} \
${OUTPUT_FILE} \
${DATA_FILE} \
${LOG_FILE} \
"${COMMAND_LINE}" > ${NEW_MANIFEST}
echo -------------------------------run ${PREFIX_NAME}${NAME}
rm ${OUTPUT_FILE} -f
echo ${SETARCH} ${ZEROVM} -M${NEW_MANIFEST}
${SETARCH} ${ZEROVM} -M${NEW_MANIFEST}
echo "stdout output >>>>>>>>>>"
cat ${OUTPUT_FILE}
echo "stderr output >>>>>>>>>>"
cat ${LOG_FILE}
