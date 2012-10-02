#!/bin/bash

if [ $# -lt 1 ]
then
	echo "required tests path argument, for example:"
	echo "sh run_test.sh tests/mkdir/00.t"
	exit
fi

prove "$1" -v
sh kill_fstest.sh

