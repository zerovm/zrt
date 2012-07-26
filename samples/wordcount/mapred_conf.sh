#!/bin/bash
MAP_NODES_LIST="1 2 3 4"
REDUCE_NODES_LIST="5 6 7 8 9"
DB_NAME=conf_mapred.sql
MAP_NODE_NAME=map.nexe
REDUCE_NODE_NAME=reduce.nexe

for number in $MAP_NODES_LIST
do
    echo $number
done

echo "PRAGMA foreign_keys=OFF;" > $DB_NAME
echo "BEGIN TRANSACTION;" >> $DB_NAME

#echo "CREATE TABLE nexes(nexeid int, name text, nexe blob);" > $DB_NAME
#echo insert as blob ./$MAP_NODE_NAME
#echo "INSERT INTO nexes (nexeid, name, blob ) VALUES (1, $MAP_NODE_NAME, '$MAP_NODE_NAME',X'$(od -A n -t x1 $MAP_NODE_NAME | tr -d '\r\n\t ')');" >> $DB_NAME 
#echo insert as blob ./$REDUCE_NODE_NAME
#echo "INSERT INTO nexes (nexeid, name, blob ) VALUES (2, $REDUCE_NODE_NAME, '$REDUCE_NODE_NAME',X'$(od -A n -t x1 $REDUCE_NODE_NAME | tr -d '\r\n\t ')');" >> $DB_NAME

echo "INSERT INTO nodes VALUES('source', 'ipc:///tmp/histograms-%d', 'w',  3);" >> $DB_NAME
