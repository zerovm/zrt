#!/bin/bash

if [ "`pgrep -f fstest.manifest`" != "" ]
then
    echo "kill fstest.nexe instance" 
    pgrep -f fstest.manifest | xargs kill
fi