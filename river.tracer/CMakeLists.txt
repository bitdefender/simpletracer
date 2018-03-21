## river.tracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

link_directories(${Z3_ROOT}/bin ${Z3_ROOT}/lib)
find_library (z3 libz3.so)

link_directories (${CMAKE_BINARY_DIR}/lib $ENV{RIVER_SDK_DIR}/lin/lib)
find_library (execution libexecution.so)
find_library (symbolicenvironment libsymbolicenvironment.so)

add_executable (river.tracer river.tracer.cpp)
target_link_libraries (river.tracer
# tracer libs
  tracer
  taintedindex
  z3executor
  z3
# dependencies
  execution
  symbolicenvironment
# native deps
  pthread
  dl
# format deps
  format.handler
  logger
  )
install (TARGETS river.tracer DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
