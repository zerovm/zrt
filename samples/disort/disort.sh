#!/bin/bash

./gendata.sh

#config for 10src, 10dst nodes
SRC_FIRST=1
SRC_LAST=10
DST_FIRST=1
DST_LAST=10

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
	gnome-terminal --geometry=80x20 -t "zerovm sortsrc$COUNTER.manifest" -x sh -c "${ZEROVM_ROOT}/zerovm -Mmanifest/sortsrc"$COUNTER".manifest"
    let COUNTER=COUNTER+1 
done

COUNTER=$DST_FIRST
while [  $COUNTER -le $DST_LAST ]; do
    gnome-terminal --geometry=80x20 -t "zerovm sortdst$COUNTER.manifest" -x sh -c "${ZEROVM_ROOT}/zerovm -Mmanifest/sortdst"$COUNTER".manifest"
    let COUNTER=COUNTER+1 
done

date > /tmp/time
${ZEROVM_ROOT}/zerovm -Mmanifest/sortman.manifest
date >> /tmp/time

cd ../samples/disort
cat log/sortman.stderr.log
echo Manager node working time: 
cat /tmp/time

