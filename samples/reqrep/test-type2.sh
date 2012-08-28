#!/bin/bash

gnome-terminal --geometry=80x20 -t "zerovm test2-mode2.manifest" -x sh -c "${ZEROVM_ROOT}/zerovm -Mtest2-mode2.manifest -v10"
${ZEROVM_ROOT}/zerovm -Mtest1-mode2.manifest -v10

sleep 1
echo "############### test 1 #################"
cat log/1stderr.log
echo "############### test 2 #################"
cat log/2stderr.log



