cmake_minimum_required (VERSION 2.8)
set (CMAKE_SYSTEM_NAME Linux)
set(CMAKE_VERBOSE_MAKEFILE on)
project (SIMPLE_TRACER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

# download libs
IF (EXISTS ${CMAKE_INSTALL_PREFIX}/lib)
	MESSAGE (STATUS "libs present. For update please $ rm -r ${CMAKE_INSTALL_PREFIX}/lib")
ELSE ()
	execute_process (COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/libs.sh teodor-stoenescu simpletracer ${RIVER_SDK_VERSION} libs.zip ${CMAKE_BINARY_DIR}/libs.zip)
	execute_process (COMMAND unzip ${CMAKE_BINARY_DIR}/libs.zip -d ${CMAKE_INSTALL_PREFIX}/lib)
	execute_process (COMMAND rm ${CMAKE_BINARY_DIR}/libs.zip)

	execute_process (COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/libs.sh teodor-stoenescu simpletracer ${RIVER_SDK_VERSION} river.sdk.zip ${CMAKE_BINARY_DIR}/river.sdk.zip)
	execute_process (COMMAND unzip ${CMAKE_BINARY_DIR}/river.sdk.zip -d ${CMAKE_CURRENT_SOURCE_DIR}/river.sdk)
	execute_process (COMMAND rm ${CMAKE_BINARY_DIR}/river.sdk.zip)
ENDIF ()

# check dependencies
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/river.sdk/include)

# build targets
add_subdirectory (simple.tracer)
add_subdirectory (annotated.tracer)
