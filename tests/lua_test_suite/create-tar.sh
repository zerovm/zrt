TEST_LIST="lua-5.2.1-tests/all.lua \
lua-5.2.1-tests/closure.lua \
lua-5.2.1-tests/files.lua \
lua-5.2.1-tests/nextvar.lua \
lua-5.2.1-tests/api.lua \
lua-5.2.1-tests/code.lua \
lua-5.2.1-tests/gc.lua \
lua-5.2.1-tests/pm.lua \
lua-5.2.1-tests/attrib.lua \
lua-5.2.1-tests/constructs.lua \
lua-5.2.1-tests/goto.lua \
lua-5.2.1-tests/sort.lua \
lua-5.2.1-tests/big.lua \
lua-5.2.1-tests/coroutine.lua \
lua-5.2.1-tests/literals.lua \
lua-5.2.1-tests/strings.lua \
lua-5.2.1-tests/bitwise.lua \
lua-5.2.1-tests/db.lua \
lua-5.2.1-tests/locals.lua \
lua-5.2.1-tests/vararg.lua \
lua-5.2.1-tests/calls.lua \
lua-5.2.1-tests/errors.lua \
lua-5.2.1-tests/main.lua \
lua-5.2.1-tests/verybig.lua \
lua-5.2.1-tests/checktable.lua \
lua-5.2.1-tests/events.lua \
lua-5.2.1-tests/math.lua"

#it's will added into tar, to use it as /tmp dir inside nexe
mkdir tmp
rm -f mounts/lua-5.2.1-tests.tar

#echo tar -cf lua-5.2.1-tests.tar ${TEST_LIST}
tar -cf lua-5.2.1-tests.tar $TEST_LIST tmp

rmdir tmp
mkdir -p mounts
mv lua-5.2.1-tests.tar mounts/lua-5.2.1-tests.tar