## Simple tracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

link_directories (${CMAKE_BINARY_DIR}/lib)
find_library (execution libexecution.so)

add_executable (simple_tracer BinLog.cpp tracer.simple.cpp Logger.cpp)
target_link_libraries (simple_tracer execution pthread dl)
install (TARGETS simple_tracer DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
