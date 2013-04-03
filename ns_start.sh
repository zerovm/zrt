#!/bin/bash
SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`

if [ $# -lt 1 ]
then
	echo "required arg: nodes count for nameserver"
	exit
fi

NAMESERVICE=ns_server.py
pkill -f ${NAMESERVICE}
echo "python ${SCRIPT_PATH}/${NAMESERVICE} $1 54321 > nameservice.log &"
python -u ${SCRIPT_PATH}/${NAMESERVICE} $1 54321 > nameservice.log &
sleep 1
