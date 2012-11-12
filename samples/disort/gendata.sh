#!/bin/bash
source ${ZRT_ROOT}/run.env

mkdir data -p

SRC_FIRST=1
SRC_LAST=10

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
    echo generate data for $COUNTER node
    ${SETARCH} ${ZEROVM} -Mmanifest/generator"$COUNTER".manifest
    cat log/"$COUNTER"generator.stderr.log
    let COUNTER=COUNTER+1 
done
