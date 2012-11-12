#!/bin/bash
source ${ZRT_ROOT}/run.env

${ZRT_ROOT}/ns_start.sh 2

${SETARCH} ${ZEROVM} -Mtest2.manifest &
${SETARCH} ${ZEROVM} -Mtest1.manifest 

sleep 1
echo "############### test 1 #################"
cat log/1stderr.log
echo "############### test 2 #################"
cat log/2stderr.log

${ZRT_ROOT}/ns_stop.sh

