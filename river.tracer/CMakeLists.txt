## river.tracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D_NO_TRACK_CALLBACKS_ -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D_NO_TRACK_CALLBACKS_ -D__cdecl=\"\" -D__stdcall=\"\"")

link_directories (${CMAKE_BINARY_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/../river.sdk/lin/lib)
find_library (execution libexecution.so)
find_library (symbolicenvironment libsymbolicenvironment.so)

add_executable (river.tracer river.tracer.cpp)
target_link_libraries (river.tracer
  libtracer
  symbolicenvironment
  execution
  pthread
  dl
  format.handler
  logger)
install (TARGETS river.tracer DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
