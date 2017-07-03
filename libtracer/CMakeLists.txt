## libtracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

add_library(libtracer STATIC
  basic.observer.cpp
  simple.tracer/simple.tracer.cpp
  annotated.tracer/TrackingExecutor.cpp
  annotated.tracer/BitMap.cpp
  annotated.tracer/annotated.tracer.cpp)

install (TARGETS libtracer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
