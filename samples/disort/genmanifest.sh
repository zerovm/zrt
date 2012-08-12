#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`/
echo $SCRIPT_PATH 

#Generate from template
SRC_FIRST=1
SRC_LAST=10
DST_FIRST=1
DST_LAST=10

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/generator.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ | \
    sed /src"$COUNTER"-src"$COUNTER"/d > manifest/generator"$COUNTER".manifest

    sed s/{NODEID}/$COUNTER/g manifest/sortsrc.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ | \
    sed /src"$COUNTER"-src"$COUNTER"/d > manifest/sortsrc"$COUNTER".manifest
    let COUNTER=COUNTER+1 
done

COUNTER=$DST_FIRST
while [  $COUNTER -le $DST_LAST ]; do
    sed s/{NODEID}/$COUNTER/g manifest/sortdst.manifest.template | \
    sed s@{ABS_PATH}@$SCRIPT_PATH@ > manifest/sortdst"$COUNTER".manifest
    let COUNTER=COUNTER+1 
done

sed s/{NODEID}/$COUNTER/g manifest/sortman.manifest.template | \
sed s@{ABS_PATH}@$SCRIPT_PATH@ |
sed s@r_man"$COUNTER"-@/dev/in/@g > manifest/sortman.manifest

sed s@{ABS_PATH}@$SCRIPT_PATH@g manifest/generator.manifest.template > manifest/generator.manifest
