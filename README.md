# Simple Tracer HOW TO

## 0. prerequisites
Set your github user token. You can get one from https://github.com/settings/tokens (when logged in)

```
$ export GITHUB_API_TOKEN=<your_github_api_token>
```

Alternately you can append the following line to your ~/.bashrc file

```
GITHUB_API_TOKEN=<your_github_api_token>
```

## 1. get sources

```
$ mkdir symexec
$ cd symexec
$ git clone https://github.com/teodor-stoenescu/simpletracer.git
```
## 2. environment setup

```
$ ln -s  `readlink -f ./simpletracer/scripts/clean_build.sh` clean_build.sh
```

## 3. build

```
$ ./clean_build.sh master 0.0.2
$ cd build
$ cmake --build .
$ cd -
```

## 4. run
```
$ cd ./build/bin
$ export LD_LIBRARY_PATH=`pwd`/../lib
$ ./simple_tracer --payload libhttp-parser.so < <input_test_case>
```

**--payload** specifies the shared object that you want to trace. This must export the payload that is the tested sw entry point.

**\<input_test_case\>** is corpus file that represents a test case.

