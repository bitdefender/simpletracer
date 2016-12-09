cmake_minimum_required (VERSION 2.8)
set (CMAKE_SYSTEM_NAME Linux)
set(CMAKE_VERBOSE_MAKEFILE on)
project (SIMPLE_TRACER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

# download libs
IF (EXISTS ${CMAKE_INSTALL_PREFIX}/lib)
	MESSAGE (STATUS "libs present. For update please $ rm -r ${CMAKE_INSTALL_PREFIX}/lib")
ELSE ()
	execute_process (COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libs.sh teodor-stoenescu simpletracer ${RIVER_SDK_VERSION} libs.zip ${CMAKE_BINARY_DIR}/libs.zip)
	execute_process (COMMAND unzip ${CMAKE_BINARY_DIR}/libs.zip -d ${CMAKE_INSTALL_PREFIX}/lib)
	execute_process (COMMAND rm ${CMAKE_BINARY_DIR}/libs.zip)
	execute_process (COMMAND find /lib -name libc.so.6 -path *i386* OUTPUT_VARIABLE LIBC_PATH)
	string (STRIP ${LIBC_PATH} LIBC_PATH)
	execute_process (COMMAND ln -s -T ${LIBC_PATH} ${CMAKE_INSTALL_PREFIX}/lib/libc.so.6)
ENDIF ()

# check dependencies
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory (tracer.simple)
