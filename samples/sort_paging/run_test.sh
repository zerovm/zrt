#!/bin/bash
source ${ZRT_ROOT}/run.env
./genmanifest.sh
echo ---------------------------------------------------- generating
time ${SETARCH} ${ZEROVM} -Mgenerator.manifest -v2
cat generator.stderr.log
echo ---------------------------------------------------- sorting
time ${SETARCH} ${ZEROVM} -Msort.manifest -v2
cat sort.stderr.log
echo ---------------------------------------------------- testing
${SETARCH} ${ZEROVM} -Mtest.manifest -v2
cat test.stderr.log

