#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`

#Generate from template
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/readdir.manifest.template  > readdir.manifest

echo some input text > some.input

echo -------------------------------run test 1
echo manifest1 test
rm 1.errlog -f
echo ${ZEROVM_ROOT}/zerovm -M${SCRIPT_PATH}/readdir.manifest
${ZEROVM_ROOT}/zerovm -M${SCRIPT_PATH}/readdir.manifest
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -M${SCRIPT_PATH}/readdir.manifest
echo "stdout output >>>>>>>>>>"
cat stdout.data





