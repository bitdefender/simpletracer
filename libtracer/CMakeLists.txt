## tracer CmakeLists.txt

set(LIBRARY_NAME tracer)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

add_library(${LIBRARY_NAME} STATIC
  utils.cpp
  basic.observer.cpp
  simple.tracer/simple.tracer.cpp
  annotated.tracer/annotated.tracer.cpp)

add_subdirectory(./annotated.tracer/tainted.index)
add_subdirectory(./annotated.tracer/z3.executor)

install (TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
