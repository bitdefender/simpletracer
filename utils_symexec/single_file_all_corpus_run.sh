#!/bin/bash
#
# Run tracer.simple for all the inputs from corpus-libhttp/CORPUS-01/
#
# Creates all_trace_simple_out/ which contains all traces.simple.out 
# from all the testcases from the above corpus and 
# all_logs.txt file which is the merge of all the files from 
# all_trace_simple_out/
#
# TODO
#
#         

cd ./build/bin
export LD_LIBRARY_PATH=`pwd`/../lib

readonly PATH_TO_CORPUS='../../corpus-libhttp/CORPUS-01/'
readonly PATH_TO_ALL_TRACES='all_trace_simple_out/'

inputs=`ls $PATH_TO_CORPUS`
outputLogFileName=""
outputAllLogs="all_logs.txt"
inputName=""
fileSize=0

if [ ! -d $PATH_TO_ALL_TRACES ]; then
  mkdir $PATH_TO_ALL_TRACES;
fi

for input in $inputs
do
    inputName="$PATH_TO_CORPUS""$input"
    echo "Runing with input $inputName"
    outputLogFileName="$PATH_TO_ALL_TRACES""$input""_out.txt"

    ./simple_tracer \
        --payload libhttp-parser.so \
        -o $outputLogFileName  < $inputName
    # fileSize=$(ls -lah $outputlogFileName | awk '{ print $5}')
    # ls -lht $outputLogFileName
    # echo "$outputLogFileName $fileSize"

    cat $outputLogFileName >> $outputAllLogs

    # TODO 
    # remove these lines 
    echo "" >> $outputAllLogs
    echo "+++++++++++++++++++++++++++" >> $outputAllLogs
    echo "" >> $outputAllLogs
    # /remove these lines 
done

echo ""
du -h $PATH_TO_ALL_TRACES
du -h $outputAllLogs
echo "Directory and file can be found in build/bin/"
