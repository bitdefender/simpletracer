## logger CmakeLists.txt

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

add_library(logger STATIC AbstractLog.cpp FileLog.cpp)

install (TARGETS logger DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
