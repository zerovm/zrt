#!/bin/bash

STDIN=data/fstest.stdin.data
STDOUT=data/fstest.stdout.data
STDERR=data/fstest.stderr.data
ZEROVM_OUTPUT=data/zerovm.output

if [ $# -lt 1 ]
then
	echo "required tests path argument, for example:"
	echo "sh run_test.sh "tests -r -f""
	exit
fi

rm -f $STDIN $STDOUT $STDERR $ZEROVM_OUTPUT
prove ""$1""
./kill_fstest.sh

