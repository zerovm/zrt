#!/bin/bash

echo Run lua samples

./genmanifest.sh lua/scripts/hello.lua log/lua_output.1 /dev/null /dev/null > lua/hello_lua.manifest 
echo -------------------------------run lua #1: hello world
rm log/lua_output.1 -f
echo ../../zvm/zerovm -Mlua/hello_lua.manifest
../../zvm/zerovm -Mlua/hello_lua.manifest
echo "stdout output >>>>>>>>>>"
cat log/lua_output.1

./genmanifest.sh lua/scripts/pngparse.lua log/lua_output.2 lua/scripts/280x.png log/lua_logfile.2 "/dev/input" > lua/pngparse_lua.manifest 
echo -------------------------------run lua #2: png parse
rm log/lua_output.2 -f
echo ../../zvm/zerovm -Mlua/pngparse_lua.manifest
../../zvm/zerovm -Mlua/pngparse_lua.manifest
echo "stderr output >>>>>>>>>>"
cat log/lua_output.2

./genmanifest.sh lua/scripts/command_line.lua log/lua_output.3 /dev/null log/lua_logfile.3 "var1 var2 var3"  > lua/command_line_lua.manifest 
echo -------------------------------run lua #3: command line
rm log/lua_output.3 -f
echo ../../zvm/zerovm -Mlua/command_line_lua.manifest
../../zvm/zerovm -Mlua/command_line_lua.manifest
echo "stderr output >>>>>>>>>>"
cat log/lua_output.3








