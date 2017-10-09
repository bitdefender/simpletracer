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
$ mkdir <work-dir>
$ cd <work-dir>
$ git clone https://github.com/teodor-stoenescu/simpletracer.git
```
## 2. environment setup

You need to set up the environment every time there's a new river release. This also builds the river tools.
```
$ ./scripts/clean_build.sh <branch-name> <release-number>
```
**\<release-number\>** is the release version needed for simpletracer build  
**\<branch-name\>** is simpletracer branch that corresponds with release-number

## 3. run
```
$ cd ./build-river-tools
$ export LD_LIBRARY_PATH=`pwd`/../river.sdk/lin/lib:`pwd`/lib
$ ./bin/river.tracer --payload <target-library> < <input_test_case>
```
**--payload \<target-library\>** specifies the shared object that you want to trace. This must export the payload that is the tested sw entry point.

**\<input_test_case\>** is corpus file that represents a test case.
