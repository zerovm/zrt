#!/bin/bash

rm data/*.res -f
rm log/* -f
./genmanifest.sh

${ZRT_ROOT}/ns_start.sh 9

#config for mapreduce network
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=1
REDUCE_LAST=5

time

COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mmanifest/map$COUNTER.manifest > /dev/null &
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
#run reduce nodes -1 count
while [  $COUNTER -lt $REDUCE_LAST ]; do
    setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mmanifest/reduce$COUNTER.manifest  > /dev/null &
    let COUNTER=COUNTER+1 
done

#run last reduce node
time setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mmanifest/reduce"$REDUCE_LAST".manifest

${ZRT_ROOT}/ns_stop.sh
