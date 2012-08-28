./genmanifest.sh
echo ---------------------------------------------------- generating
time ${ZEROVM_ROOT}/zerovm -Mgenerator.manifest -v2
cat generator.stderr.log
echo ---------------------------------------------------- sorting
time ${ZEROVM_ROOT}/zerovm -Msort.manifest -v2
cat sort.stderr.log
echo ---------------------------------------------------- testing
${ZEROVM_ROOT}/zerovm -Mtest.manifest -v2
cat test.stderr.log

