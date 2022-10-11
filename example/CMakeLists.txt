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

add_library(eventrouter STATIC ../eventrouter.c)
target_include_directories(eventrouter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(eventrouter PUBLIC freertos_kernel)

add_library(task_generic STATIC task_generic.c)
target_link_libraries(task_generic PUBLIC eventrouter)

add_library(module_sensor_data_publisher STATIC module_sensor_data_publisher.c)
target_link_libraries(module_sensor_data_publisher PUBLIC eventrouter)

add_library(module_data_logger STATIC module_data_logger.c)
target_link_libraries(module_data_logger PUBLIC
  eventrouter
  module_sensor_data_publisher
)

add_library(module_data_uploader STATIC module_data_uploader.c)
target_link_libraries(module_data_uploader PUBLIC
  eventrouter
  module_sensor_data_publisher
)

add_executable(main main.c)
target_link_libraries(main PRIVATE
  eventrouter
  module_data_logger
  module_data_uploader
  module_sensor_data_publisher
  task_generic
)
