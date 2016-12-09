#!/bin/bash
#
# Run a testcase from corpus-libhttp/CORPUS-01/
#

cd ./build/bin
export LD_LIBRARY_PATH=`pwd`/../lib
./simple_tracer --payload libhttp-parser.so < ../../corpus-libhttp/CORPUS-01/faa8e7d7d6dd27276bac15183dcdfe43eae39633 

executionLogFileName="execution.log"
traceSimpleOutFileName="trace.simple.out"

echo ""
du -h $executionLogFileName $traceSimpleOutFileName
echo "execution.log and trace.simple.out can be found in build/bin/"
