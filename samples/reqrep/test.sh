#!/bin/bash

${ZRT_ROOT}/ns_start.sh 2

setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mtest2.manifest &
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mtest1.manifest 

sleep 1
echo "############### test 1 #################"
cat log/1stderr.log
echo "############### test 2 #################"
cat log/2stderr.log

${ZRT_ROOT}/ns_stop.sh

