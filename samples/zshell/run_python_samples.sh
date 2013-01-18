#!/bin/bash
source ${ZRT_ROOT}/run.env

echo Run python samples

PREFIX_NAME="python_"
NAME="hello"
SCRIPT_PATH_ON_REAL_FS=python/scripts/${NAME}.py
OUTPUT_FILE=log/${PREFIX_NAME}${NAME}.stdout
DATA_FILE=/dev/null
LOG_FILE=log/${PREFIX_NAME}${NAME}.stderr.log
#COMMAND_LINE="-v -d /dev/stdin"
COMMAND_LINE="/dev/stdin test1 test2"
NEW_MANIFEST=python/${NAME}.manifest

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

####################################################
PREFIX_NAME="python_"
NAME="pickle"
SCRIPT_PATH_ON_REAL_FS=python/scripts/${NAME}.py
OUTPUT_FILE=log/${PREFIX_NAME}${NAME}.stdout
DATA_FILE=/dev/null
LOG_FILE=log/${PREFIX_NAME}${NAME}.stderr.log
COMMAND_LINE="/dev/stdin"
NEW_MANIFEST=python/${NAME}.manifest

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
