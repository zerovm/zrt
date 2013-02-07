#!/bin/bash

FIFO_FOR_INPUT=data/fstest.fifo.input

#Close background fstest.nexe process or kill it if did not respond

if [ "`pgrep -f fstest.manifest`" != "" ]
then
    echo "kill fstest.nexe instance"
    #send controldata to kill fstest 
    echo test12345complete > $FIFO_FOR_INPUT
    sleep 1
    if [ "`pgrep -f fstest.manifest`" != "" ]
    then
        echo fstest does not closed correctly - killed
        pgrep -f fstest.manifest | xargs kill
    fi
fi