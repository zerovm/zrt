#!/bin/bash

if [ $# -lt 1 ]
then
	echo "required arg: nodes count for nameserver"
	exit
fi

NAMESERVICE=ns_server.py
pkill -f ${NAMESERVICE}
echo "python ${ZRT_ROOT}/${NAMESERVICE} $1 54321 > nameservice.log &"
python ${ZRT_ROOT}/${NAMESERVICE} $1 54321 > nameservice.log &

