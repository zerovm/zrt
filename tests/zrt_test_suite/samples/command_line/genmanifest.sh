#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`

#Generate from template

sed s@{ABS_PATH}@$SCRIPT_PATH/@g command_line.manifest.template > command_line.manifest



