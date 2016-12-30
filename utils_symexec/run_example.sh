#!/bin/bash
#
# Run a testcase from $LD_CORPUS_PATH.
#
# $LD_CORPUS_PATH is defined in stub_env_variables.sh.
#

cd ./build/bin

files=($LD_CORPUS_PATH/*)
testcase="${files[RANDOM % ${#files[@]}]}"

printf "Using %s testcase.\n" "$testcase"
./simple_tracer --payload libhttp-parser.so < $testcase

executionLogFileName="execution.log"
traceSimpleOutFileName="trace.simple.out"

echo ""
du -h $executionLogFileName $traceSimpleOutFileName
echo "execution.log and trace.simple.out can be found in build/bin/"
