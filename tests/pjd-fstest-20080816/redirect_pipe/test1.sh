#!/bin/bash

#Test redirect_pipe
#Both pipes are not directly linked and using file for IPC

TEMP_PIPE=temp.p

pkill redirect_pipe

#create pipe for reading into tst-res.txt
touch ipc_file.txt
./redirect_pipe -f ipc_file.txt -r > tst-res.txt &

#create pipe $TEMP_PIPE for writing test files
rm -f $TEMP_PIPE
mkfifo $TEMP_PIPE
cat $TEMP_PIPE | ./redirect_pipe -f ipc_file.txt -w &
cat tst.txt > $TEMP_PIPE
cat tst2.txt >> $TEMP_PIPE

pkill redirect_pipe
rm -f $TEMP_PIPE ipc_file.txt

diff -q tst-res.txt tst-sum.txt
if [ "$?" = "0" ]
then
rm -f tst-res.txt
echo "test OK"
exit 0;
else
rm -f tst-res.txt
echo "test FAILED"
exit 1;
fi

