#!/bin/bash

./debug/gcc/copy.sh
./gcc_map 1 1>log/map1.stdout 2> log/map1.stderr.log
./debug/gcc/getmovemap1files.sh

./debug/gcc/copy.sh
./gcc_map 2 1>log/map2.stdout 2> log/map2.stderr.log
./debug/gcc/getmovemap2files.sh

./debug/gcc/copy.sh
./gcc_map 3 1>log/map3.stdout 2> log/map3.stderr.log
./debug/gcc/getmovemap3files.sh

./debug/gcc/copy.sh
./gcc_map 4 1>log/map4.stdout 2> log/map4.stderr.log
./debug/gcc/getmovemap4files.sh

./debug/gcc/copy.sh

echo Launch again if you see errors
