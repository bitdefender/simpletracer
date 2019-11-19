# Simple Tracer HOW TO

## 1. get sources

```
$ mkdir <work-dir>
$ cd <work-dir>
$ git clone https://github.com/bitdefender/simpletracer.git
```
## 2. environment setup

Simpletracer build needs RIVER SDK. Export `RIVER_SDK_DIR` accordingly.  
Simpletracer supports tracing using Z3 engine. Install Z3 4.4.1 32-bit
and export `Z3_ROOT_PATH` accordingly.  
Build Simpletracer using `cmake`.

`LD_LIBRARY_PATH` must allow access to RIVER libraries, Z3 library and
native libraries `libc.so` and `libpthread.so`.  
Create the following symlinks:
```
RIVER_NATIVE_LIBS=<some path>
LIBC_PATH=$(find /lib -name libc.so.6 -path *i386*)
LIBPTHREAD_PATH=$(find /lib -name libpthread.so.0 -path *i386*)
ln -s -T $LIBC_PATH $RIVER_NATIVE_LIBS/libc.so
ln -s -T $LIBPTHREAD_PATH $RIVER_NATIVE_LIBS/libpthread.so
```

## 3. run
```
$ ./bin/river.tracer --payload <target-library> [--annotated] [--z3] < <input_test_case>
```
**--payload \<target-library\>** specifies the shared object that you want to trace. This must export the payload that is the tested sw entry point.

**--annotated** adds tainted index data in traces

**--z3** specified together with `--annotated` adds z3 data in traces

**\<input_test_case\>** is corpus file that represents a test case.
