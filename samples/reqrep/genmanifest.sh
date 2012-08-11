#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`

#Generate from template

sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/test1.manifest.template        > test1.manifest
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/test1-mode2.manifest.template  > test1-mode2.manifest
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/test2.manifest.template        > test2.manifest
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/test2-mode2.manifest.template  > test2-mode2.manifest  



