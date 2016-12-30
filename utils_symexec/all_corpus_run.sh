#!/bin/bash
#
# Run simple_tracer for all the inputs from $LD_CORPUS_PATH.
# 
# $LD_LIBRARY_PATH, 
# $LD_CORPUS_PATH, 
# $LD_TRACES_PATH 
# defined in stub_env_variables.sh
#
# Creates all_trace_simple_out/ which contains all traces.simple.out 
# from all the testcases from the above corpus. 
# 
# TODO
#
#         

cd ./build/bin

inputs=`ls $LD_CORPUS_PATH`
outputLogFileName=""
inputName=""
fileSize=0

if [ ! -d $LD_TRACES_PATH ]; then
  mkdir $LD_TRACES_PATH;
fi

for input in $inputs
do
    inputName="$LD_CORPUS_PATH""$input"
    echo "Runing with input $inputName"
    outputLogFileName="$LD_TRACES_PATH""$input""_out.txt"

    ./simple_tracer \
        --payload libhttp-parser.so \
        -o $outputLogFileName  < $inputName
    # fileSize=$(ls -lah $outputlogFileName | awk '{ print $5}')
    # ls -lht $outputLogFileName
    # echo "$outputLogFileName $fileSize"
    echo ""
done

echo ""
du -h $LD_TRACES_PATH
echo "Directory can be found in build/bin/"
