# Overview:
#
#   This folder contains implementation-specific tests/applications and
#   implementation-agnostic tests. This requires building the eventrouter
#   library for each implementation.
#
# Convention:
#
#   Libraries, sources, and tests which are implementation-specific should start
#   with the implementation name. For example, baremetal_* or freertos_*, etc.

cmake_minimum_required(VERSION 3.17)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(cmake-extras)

include(cmake/repository_root.cmake)
include(cmake/freertos.cmake)
include(cmake/googletest.cmake)

#===============================================================================
# Build implementation-specific eventrouter libs; used by examples and tests.
# ===============================================================================

add_library(baremetal_eventrouter STATIC ${REPOSITORY_ROOT}/eventrouter.c)
target_include_directories(baremetal_eventrouter PUBLIC ${REPOSITORY_ROOT}  .)
target_compile_definitions(baremetal_eventrouter PUBLIC -DER_BAREMETAL)

add_library(freertos_eventrouter STATIC ${REPOSITORY_ROOT}/eventrouter.c)
target_include_directories(freertos_eventrouter PUBLIC ${REPOSITORY_ROOT} .)
target_link_libraries(freertos_eventrouter PUBLIC freertos_kernel)
target_compile_definitions(freertos_eventrouter PUBLIC -DER_FREERTOS)

#===============================================================================
# Build tests and example applications
#===============================================================================

add_subdirectory(example)
add_subdirectory(test)
