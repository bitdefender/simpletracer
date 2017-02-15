#!/bin/bash
#
# Run tracer.simple for all the inputs from $LD_CORPUS_PATH.
# 
# $LD_LIBRARY_PATH, 
# $LD_CORPUS_PATH, 
# $LD_TRACES_PATH 
# defined in stub_env_variables.sh
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

inputs=`ls $LD_CORPUS_PATH`
outputLogFileName=""
outputAllLogs="all_logs.txt"
inputName=""
fileSize=0

if [ ! -d $LD_TRACES_PATH ]; then
  mkdir $LD_TRACES_PATH;
fi

echo "" > $outputAllLogs

for input in $inputs
do
    inputName="$LD_CORPUS_PATH""$input"
    echo "Runing with input $inputName"
    outputLogFileName="$LD_TRACES_PATH""$input""_out.txt"
    
    echo $LD_TRACES_PATH
    echo $outputLogFileName
    
    
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
du -h $LD_TRACES_PATH
du -h $outputAllLogs
echo "Directory and file can be found in build/bin/"
