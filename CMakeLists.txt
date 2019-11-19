cmake_minimum_required (VERSION 2.8)
set (CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_VERBOSE_MAKEFILE on)
project (SIMPLE_TRACER)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

# check dependencies
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories($ENV{RIVER_SDK_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/river.format/include)

# build targets
add_subdirectory (river.format/logger)

add_subdirectory (river.format/format.handler)

execute_process(
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/z3
	COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/z3/scripts/mk_make.py --prefix=${CMAKE_INSTALL_PREFIX}
	)
execute_process(
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/z3/build
	COMMAND make
	)
execute_process(
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/z3/build
	COMMAND make install
	)

add_subdirectory (libtracer)
add_subdirectory (river.tracer)
add_subdirectory (offline.sanitizers)
