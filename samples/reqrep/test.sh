#!/bin/bash

cd ../../zvm

gnome-terminal --geometry=80x20 -t "zerovm test1.manifest" -x sh -c "./zerovm -M../samples/reqrep/test1.manifest"
./zerovm -M../samples/reqrep/test2.manifest

sleep 1
echo "############### test 1 #################"
cat ../samples/reqrep/log/test1.stderr.log
echo "############### test 2 #################"
cat ../samples/reqrep/log/test2.stderr.log



