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
include(cmake/googletest.cmake)

#===============================================================================
# Select implementation.
#===============================================================================

set(IMPLEMENTATION "baremetal" CACHE STRING "Select the implementation to build")
set(ALLOWED_IMPLEMENTATIONS "baremetal;freertos;posix")
set_property(CACHE IMPLEMENTATION PROPERTY STRINGS ${ALLOWED_IMPLEMENTATIONS})
if(NOT IMPLEMENTATION IN_LIST ALLOWED_IMPLEMENTATIONS)
    message(FATAL_ERROR "IMPLEMENTATION must be one of: ${ALLOWED_IMPLEMENTATIONS}")
endif()

#===============================================================================
# Build the eventrouter library; used by examples and tests.
# ===============================================================================

add_library(eventrouter STATIC ${REPOSITORY_ROOT}/eventrouter.c)
target_include_directories(eventrouter PUBLIC ${REPOSITORY_ROOT} .)

if(IMPLEMENTATION STREQUAL "baremetal")
    target_compile_definitions(eventrouter PUBLIC -DER_BAREMETAL)
elseif(IMPLEMENTATION STREQUAL "freertos")
    include(cmake/freertos.cmake)
    target_link_libraries(eventrouter PUBLIC freertos_kernel)
    target_compile_definitions(eventrouter PUBLIC -DER_FREERTOS)
elseif(IMPLEMENTATION STREQUAL "posix")
    target_compile_definitions(eventrouter PUBLIC -DER_POSIX)
endif()

#===============================================================================
# Build tests and example applications
#===============================================================================

add_subdirectory(example)
add_subdirectory(test)
