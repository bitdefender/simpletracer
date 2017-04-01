## Simple tracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

link_directories (${CMAKE_BINARY_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/../river.sdk/lin/lib)
find_library (execution libexecution.so)

add_executable (simple_tracer AbstractLog.cpp FileLog.cpp BinFormat.cpp TextFormat.cpp simple.tracer.cpp Logger.cpp)
target_link_libraries (simple_tracer execution pthread dl)
install (TARGETS simple_tracer DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
