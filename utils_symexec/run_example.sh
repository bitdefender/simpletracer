#!/bin/bash
#
# Run a testcase from corpus-libhttp/CORPUS-01/
#

cd ./build/bin
export LD_LIBRARY_PATH=`pwd`/../lib

files=(../../corpus-libhttp/CORPUS-01/*)
testcase="${files[RANDOM % ${#files[@]}]}"

printf "Using %s testcase.\n" "$testcase"
./simple_tracer --payload libhttp-parser.so < $testcase

executionLogFileName="execution.log"
traceSimpleOutFileName="trace.simple.out"

echo ""
du -h $executionLogFileName $traceSimpleOutFileName
echo "execution.log and trace.simple.out can be found in build/bin/"

