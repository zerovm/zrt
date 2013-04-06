#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`/
echo $SCRIPT_PATH 

SINGLE_NODE_INPUT_RECORDS_COUNT=1000000

#Generate from template
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=1
REDUCE_LAST=4

DATA_OFFSET=0
SEQUENTIAL_ID=1

#do relace and delete self communication map channels
COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
#genmanifest
    sed s/{NODEID}/$COUNTER/g manifest/map.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ | \
    sed /map"$COUNTER"-map-"$COUNTER"/d | \
    sed s@w_map"$COUNTER"-@/dev/out/@g | \
    sed s@{SEQUENTIAL_ID}@$SEQUENTIAL_ID@g | \
    sed s@r_map"$COUNTER"-@/dev/in/@g > manifest/map"$COUNTER".manifest 
#gendata
    ./gensort -c -s -b$DATA_OFFSET -a $SINGLE_NODE_INPUT_RECORDS_COUNT data/"$COUNTER"input.txt 2> data/"$COUNTER"source.sum
#increment
    let SEQUENTIAL_ID=SEQUENTIAL_ID+1
    let COUNTER=COUNTER+1 
    let DATA_OFFSET=DATA_OFFSET+SINGLE_NODE_INPUT_RECORDS_COUNT
done
cat data/*source.sum > data/in.sum
./valsort -s data/in.sum > data/in.sum

COUNTER=$REDUCE_FIRST
while [  $COUNTER -le $REDUCE_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/reduce.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ | \
    sed s@{SEQUENTIAL_ID}@$SEQUENTIAL_ID@g | \
    sed s@r_red"$COUNTER"-@/dev/in/@g > manifest/reduce"$COUNTER".manifest
    let SEQUENTIAL_ID=SEQUENTIAL_ID+1
    let COUNTER=COUNTER+1 
done


