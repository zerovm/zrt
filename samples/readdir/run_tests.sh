#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`
source ${ZRT_ROOT}/run.env

#Generate from template
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/readdir.manifest.template  > readdir.manifest

echo some input text > some.input

echo -------------------------------run test 1
echo manifest1 test
rm 1.errlog -f
echo ${ZEROVM} -M${SCRIPT_PATH}/readdir.manifest
${ZEROVM} -M${SCRIPT_PATH}/readdir.manifest
${SETARCH} ${ZEROVM} -M${SCRIPT_PATH}/readdir.manifest
echo "stdout output >>>>>>>>>>"
cat stdout.data

