# sanitizers CMakeLists.txt

set(EXECUTABLE_NAME river.sanitizer)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 -D__cdecl=\"\" -D__stdcall=\"\"")

include_directories(./include)

link_directories(${Z3_ROOT}/bin ${Z3_ROOT}/lib)
find_library (z3 libz3.so)

add_subdirectory(address.sanitizer)
#add_subdirectory(memory.sanitizer)

add_executable(${EXECUTABLE_NAME}
	Z3Handler.cpp
	TraceParser.cpp
	SanitizerMain.cpp
	)

target_link_libraries(${EXECUTABLE_NAME}
	asan
	z3
	)

install (TARGETS ${EXECUTABLE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)



