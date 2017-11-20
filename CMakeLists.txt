cmake_minimum_required (VERSION 2.8)
set (CMAKE_SYSTEM_NAME Linux)
set(CMAKE_VERBOSE_MAKEFILE on)
project (SIMPLE_TRACER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)

# check dependencies
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories($ENV{RIVER_SDK_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/river.format/include)

# build targets
add_subdirectory (river.format/logger)
add_subdirectory (river.format/format.handler)

if (NOT DEFINED ENV{Z3_ROOT_PATH})
  if (NOT DEFINED Z3_ROOT)
    message(FATAL_ERROR "Specify Z3_ROOT variable")
  endif()
else()
  set(Z3_ROOT $ENV{Z3_ROOT_PATH})
endif()

add_subdirectory (libtracer)
add_subdirectory (river.tracer)
