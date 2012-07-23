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
REDUCE_FIRST=5
REDUCE_LAST=8

time

COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    gnome-terminal --geometry=80x20 -t "map$COUNTER" -x sh -c "./zerovm -M../samples/wordcount/manifest/map$COUNTER.manifest"
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
while [  $COUNTER -le $REDUCE_LAST ]; do
    gnome-terminal --geometry=80x20 -t "reduce$COUNTER" -x sh -c "./zerovm -M../samples/wordcount/manifest/reduce$COUNTER.manifest"
    let COUNTER=COUNTER+1 
done

time ./zerovm -M../samples/wordcount/manifest/reduce9.manifest


