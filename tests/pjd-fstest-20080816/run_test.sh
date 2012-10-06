#!/bin/bash

STDIN=fstest.stdin.data
STDOUT=fstest.stdout.data
STDERR=fstest.stderr.data

if [ $# -lt 1 ]
then
	echo "required tests path argument, for example:"
	echo "sh run_test.sh "tests -r -f""
	exit
fi

rm -f $STDIN $STDOUT $STDERR
prove ""$1""
sh kill_fstest.sh

