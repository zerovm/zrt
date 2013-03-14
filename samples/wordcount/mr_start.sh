#!/bin/bash
source ${ZRT_ROOT}/run.env

rm data/*.res -f
rm log/* -f
./genmanifest.sh

ZVM_REPORT=report.txt

#config for mapreduce network
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=1
REDUCE_LAST=5

#calculate number of nodes for whole cluster
let NUMBER_OF_NODES=${MAP_LAST}+${REDUCE_LAST}

${ZRT_ROOT}/ns_start.sh ${NUMBER_OF_NODES}

rm ${ZVM_REPORT} -f
time

COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    ${SETARCH} ${ZEROVM} -Mmanifest/map$COUNTER.manifest >> ${ZVM_REPORT} &
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
#run reduce nodes -1 count
while [  $COUNTER -lt $REDUCE_LAST ]; do
    ${SETARCH} ${ZEROVM} -Mmanifest/reduce$COUNTER.manifest >> ${ZVM_REPORT} &
    let COUNTER=COUNTER+1 
done

#run last reduce node
time ${SETARCH} ${ZEROVM} -Mmanifest/reduce"$REDUCE_LAST".manifest >> ${ZVM_REPORT}

${ZRT_ROOT}/ns_stop.sh
sleep 1
cat ${ZVM_REPORT}
