#!/bin/bash

rm data/*.res -f
rm log/* -f

#config for mapreduce network
MAP_FIRST=1
MAP_LAST=4
REDUCE_FIRST=5
REDUCE_LAST=8

COUNTER=$MAP_FIRST
while [  $COUNTER -le $MAP_LAST ]; do
    gnome-terminal --geometry=80x20 -t "map$COUNTER" -x sh -c "./gcc_map $COUNTER 1>log/map$COUNTER.stdout 2> log/map$COUNTER.stderr.log"
    let COUNTER=COUNTER+1 
done

COUNTER=$REDUCE_FIRST
while [  $COUNTER -le $REDUCE_LAST ]; do
    gnome-terminal --geometry=80x20 -t "reduce$COUNTER" -x sh -c "./gcc_reduce $COUNTER 1>log/reduce$COUNTER.stdout 2>log/reduce$COUNTER.stderr.log"
    let COUNTER=COUNTER+1 
done

time ./gcc_reduce 9 1>log/reduce$COUNTER.stdout 2>log/reduce9.stderr.log
cat log/reduce9.stderr.log

