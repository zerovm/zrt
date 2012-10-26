#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`/
echo $SCRIPT_PATH 

#Generate from template
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=1
REDUCE_LAST=5

SEQUENTIAL_ID=1

#do relace and delete self communication map channels
COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/map.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ | \
    sed /map"$COUNTER"-map-"$COUNTER"/d | \
    sed s@w_map"$COUNTER"-@/dev/out/@g | \
    sed s@{SEQUENTIAL_ID}@$SEQUENTIAL_ID@g | \
    sed s@r_map"$COUNTER"-@/dev/in/@g > manifest/map"$COUNTER".manifest 
    let SEQUENTIAL_ID=SEQUENTIAL_ID+1
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
while [  $COUNTER -le $REDUCE_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/reduce.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ | \
    sed s@{SEQUENTIAL_ID}@$SEQUENTIAL_ID@g | \
    sed s@r_red"$COUNTER"-@/dev/in/@g > manifest/reduce"$COUNTER".manifest
    let SEQUENTIAL_ID=SEQUENTIAL_ID+1
    let COUNTER=COUNTER+1 
done


