#!/bin/bash

if [ "`pgrep -f fstest.manifest`" != "" ]
then
    echo "kill fstest.nexe instance"
    #send controldata to kill fstest 
    echo test12345complete >> fstest.stdin.data
    sleep 1
    if [ "`pgrep -f fstest.manifest`" != "" ]
    then
        echo fstest does not closed correctly - killed
        pgrep -f fstest.manifest | xargs kill
    fi
fi