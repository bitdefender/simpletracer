## libtracer CmakeLists.txt

if (NOT DEFINED ENV{Z3_ROOT_PATH})
  if (NOT DEFINED Z3_ROOT)
    message(FATAL_ERROR "Specify Z3_ROOT variable")
  endif()
else()
  set(Z3_ROOT $ENV{Z3_ROOT_PATH})
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

add_library(libtracer STATIC
  utils.cpp
  basic.observer.cpp
  simple.tracer/simple.tracer.cpp
  annotated.tracer/TaintedIndex.cpp
  annotated.tracer/TrackingExecutor.cpp
  annotated.tracer/z3/Z3SymbolicFlags.cpp
  annotated.tracer/z3/Z3SymbolicExecutor.cpp
  annotated.tracer/annotated.tracer.cpp)

include_directories(${Z3_ROOT}/include)

install (TARGETS libtracer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
