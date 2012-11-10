#!/bin/bash

SCRIPT=$(readlink -f "$0")
SCRIPT_PATH=`dirname "$SCRIPT"`
LUA_TEST_PATH=$SCRIPT_PATH/lua-5.2.1-tests

#create tarfs
sh create-tar.sh

#Generate from template
sed s@{ABS_PATH}@$SCRIPT_PATH/@g manifest_template/lua.manifest.template | \
sed s@{LUA_TEST_PATH}@$LUA_TEST_PATH/@g > lua.manifest

echo some input text > lua.input
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -M$SCRIPT_PATH/lua.manifest
cat lua.output



