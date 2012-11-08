#!/bin/bash

#close input file holder, instead of cahracter device
pkill input_file

#Close background fstest.nexe process or kill it if did not respond

if [ "`pgrep -f fstest.manifest`" != "" ]
then
    echo "kill fstest.nexe instance"
    #send controldata to kill fstest 
    echo test12345complete >> data/fstest.stdin.data
    sleep 1
    if [ "`pgrep -f fstest.manifest`" != "" ]
    then
        echo fstest does not closed correctly - killed
        pgrep -f fstest.manifest | xargs kill
    fi
fi