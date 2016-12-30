Just copy these script files in `symexec/`. 

## Environment variables  

    $ source stub_env_variables.sh [options]

where <i>options</i> could be 
    
    --default 
      Consider the default example. 

If it is used without <i>--default</i>, the user 
will be prompted to specify the path to be considered 
for the environment variables. 

## Generate traces and log files 
Based on the testcases from the CORPUS directory, 
simple_tracer will generate <i>traces</i> and <i> log files</i> 
accordingly. 

Run simple_tracer for a random testcase. 

    $ ./run_example.sh 

Run for all the inputs from CORPUS/ 

    $ ./all_corpus_run.sh

Similar with previous one, but puts all the logs generated in a single file.

    $ ./single_file_all_corpus_run.sh 


