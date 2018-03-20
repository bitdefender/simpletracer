## SymbolicDemo CMakeLists.txt

set(EXECUTABLE_NAME symbolic_demo)
set(LIBRARY_NAME z3executor)
set(CROSS_FLAGS "-D__cdecl=\"\" -D__stdcall=\"\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m32 -std=c++11 ${CROSS_FLAGS}")

include_directories($ENV{RIVER_SDK_DIR}/include)

link_directories (${CMAKE_BINARY_DIR}/lib $ENV{RIVER_SDK_DIR}/lin/lib)
find_library (symbolicenvironment libsymbolicenvironment.so)
find_library (execution libexecution.so)

link_directories(${Z3_ROOT}/bin ${Z3_ROOT}/lib)
find_library (z3 libz3.so)

link_directories(${SMTLIB2_INSTALL}/lib)
find_library(smtlib2parser libsmtlib2parser.a)

add_executable(${EXECUTABLE_NAME}
	SymbolicDemoMain.cpp
	TrackingCookie.cpp
	Payload.cpp
	)

add_library(${LIBRARY_NAME}
	LargeStack.cpp
	Z3SymbolicExecutor.cpp
	Z3SymbolicFlags.cpp
	)

target_link_libraries(${EXECUTABLE_NAME}
	${LIBRARY_NAME}
	format.handler
	logger
	tracer
	)

target_link_libraries(${LIBRARY_NAME}
	z3
	symbolicenvironment
	execution
	)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
