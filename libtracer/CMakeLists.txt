## tracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

include_directories(${Z3_ROOT}/include)

add_library(tracer STATIC
  utils.cpp
  basic.observer.cpp
  simple.tracer/simple.tracer.cpp
  annotated.tracer/TaintedIndex.cpp
  annotated.tracer/TrackingExecutor.cpp
  annotated.tracer/z3/LargeStack.cpp
  annotated.tracer/z3/Z3SymbolicFlags.cpp
  annotated.tracer/z3/Z3SymbolicExecutor.cpp
  annotated.tracer/annotated.tracer.cpp)

install (TARGETS tracer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
