# Simple Tracer HOW TO

## 0. prerequisites
Set your github user token. You can get one from https://github.com/settings/tokens (when logged in)

```$ export GITHUB_API_TOKEN=<your_github_api_token>```

Alternately you can append the following line to your ~/.bashrc file

```GITHUB_API_TOKEN=<your_github_api_token>```

## 1. get sources
```
$ git clone https://github.com/teodor-stoenescu/simpletracer.git
$ cd simpletracer
```

Before building you must bring in some dependencies, this is all automated.
First install some dependent packages (this requires root priviledges)
```
$ make deps
<type your account password; you must be on the sudoers list>
```
Next you must download the river binary component

```$ make libs```

## 2. build
```$ make clean && make```

## 3. install
```$ make install prefix=`pwd`/inst```

## 4. run
```
$ cd ./inst/bin
$ export LD_LIBRARY_PATH=`pwd`/../lib
$ ./tracer.simple --payload libhttp-parser.so < <input_test_case>
```

**--payload** specifies the shared object that you want to trace. This must export the payload that is the tested sw entry point.

**<input_test_case>** is corpus file that represents a test case.

