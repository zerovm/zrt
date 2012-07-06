echo ---------------------------------------------------- generating
../../zvm/zerovm -Mgenerator.manifest -v2
cat generator.stderr.log
echo ---------------------------------------------------- sorting
../../zvm/zerovm -Msort.manifest -v2
cat sort.stderr.log
echo ---------------------------------------------------- testing
../../zvm/zerovm -Mtest.manifest -v2
cat tester.stderr.log

