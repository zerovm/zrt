#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`/
echo $SCRIPT_PATH 

#Generate from template
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=5
REDUCE_LAST=9

COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/map.manifest.template | sed s@{ABS_PATH}@$SCRIPT_PATH@ | sed /map"$COUNTER"_map"$COUNTER"/d > manifest/map"$COUNTER".manifest 
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
while [  $COUNTER -le $REDUCE_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/reduce.manifest.template | sed s@{ABS_PATH}@$SCRIPT_PATH@ > manifest/reduce"$COUNTER".manifest
    let COUNTER=COUNTER+1 
done


