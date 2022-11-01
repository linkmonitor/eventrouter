cmake_minimum_required(VERSION 3.17)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(posix-example)

include(FetchContent)

FetchContent_Declare(
  freertos
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel
  GIT_TAG main
)

set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "")
set(FREERTOS_HEAP 4 CACHE STRING "")
set(FREERTOS_PORT GCC_POSIX CACHE STRING "")

FetchContent_MakeAvailable(freertos)
target_link_options(freertos_kernel PUBLIC
  $<$<PLATFORM_ID:Linux>:-pthread>
)

add_library(eventrouter STATIC ../../eventrouter.c)
target_include_directories(eventrouter PUBLIC ../..)
target_link_libraries(eventrouter PUBLIC freertos_kernel)

# Pull in the eventrouter modules which are common to all examples.
add_subdirectory(../common_modules common_modules)

add_library(task_generic STATIC task_generic.c)
target_link_libraries(task_generic PUBLIC eventrouter)

add_executable(main main.c)
target_link_libraries(main PRIVATE
  eventrouter
  module_data_logger
  module_data_uploader
  module_sensor_data_publisher
  task_generic
)
