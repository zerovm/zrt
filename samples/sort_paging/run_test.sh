./genmanifest.sh
echo ---------------------------------------------------- generating
time setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mgenerator.manifest -v2
cat generator.stderr.log
echo ---------------------------------------------------- sorting
time setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Msort.manifest -v2
cat sort.stderr.log
echo ---------------------------------------------------- testing
setarch x86_64 -R ${ZEROVM_ROOT}/zerovm -Mtest.manifest -v2
cat test.stderr.log

