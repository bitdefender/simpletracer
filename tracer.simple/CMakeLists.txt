## Simple tracer CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

link_directories (${CMAKE_BINARY_DIR}/lib)
find_library (execution libexecution.so)
find_library (disablesse libdisablesse.so)

add_executable (simple_tracer AbstractLog.cpp FileLog.cpp BinFormat.cpp TextFormat.cpp Main.cpp)
target_link_libraries (simple_tracer execution disablesse pthread dl)
install (TARGETS simple_tracer DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
