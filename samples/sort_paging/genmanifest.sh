#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`

#Generate from template

sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/generator.manifest.template > generator.manifest
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/sort.manifest.template      > sort.manifest
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/test.manifest.template      > test.manifest  



