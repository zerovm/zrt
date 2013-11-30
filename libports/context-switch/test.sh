#!/bin/bash
ZVSH="python zvsh"

for file in tst-*
do
	OUT=tests/$file-output.txt
	INPUT=tests/$file-input.txt

	$ZVSH $file > $OUT
	diff $INPUT $OUT > /dev/null 2>&1

	if [ $? == 0 ]; then
		echo -e "\033[01;32mPassed\033[00m"
	else
		echo -e "\033[01;31mFailed\033[00m"
		echo -e "\033[01;37mExpected output:\033[00m"
		cat $INPUT
		echo -e "\033[01;37mActual output:\033[00m"
		cat $OUT
	fi
	rm $OUT
done