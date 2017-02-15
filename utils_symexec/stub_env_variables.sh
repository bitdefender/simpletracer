#!/bin/bash
#
#     LD_LIBRARY_PATH, 
#     LD_CORPUS_PATH, 
#     LD_TRACES_PATH 
# environment variables
# needed for generating build/bin/trace.simply.out and 
# 			build/bin/execution.log. 
# 

cd build/bin

if [ "$1" == "--default" ]; then
  export LD_LIBRARY_PATH=`pwd`/../lib
  export LD_CORPUS_PATH="../../corpus-libhttp/CORPUS-01/"
else 
  echo "LD_LIBRARY_PATH? "
  read LIBRARY_PATH
  
  echo "LD_CORPUS_PATH? "
  read CORPUS_PATH
  
  # TODO
  #   check paths are valid 
  
  export LD_LIBRARY_PATH=$LIBRARY_PATH
  export LD_CORPUS_PATH=$CORPUS_PATH
fi

export LD_TRACES_PATH="all_trace_simple_out/"

cd ../..
