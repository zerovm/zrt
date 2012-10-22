#!/bin/bash

mkdir debug/zerovm/files
rm data/*.res
rm log/* -f
rm debug/zerovm/files/* -f
./genmanifest.sh
#files for reading specified in manifest should be exist for a start moment
./debug/zerovm/touches_read.sh
cd ../../zvm

#config for mapreduce network
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=1
REDUCE_LAST=5

time

COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -M../samples/wordcount/manifest/map$COUNTER.manifest > /dev/null &
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
while [  $COUNTER -le $REDUCE_LAST ]; do
    setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -M../samples/wordcount/manifest/reduce$COUNTER.manifest  > /dev/null &
    let COUNTER=COUNTER+1 
done

time setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -M../samples/wordcount/manifest/reduce"$REDUCE_LAST".manifest


