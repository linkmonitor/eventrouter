cmake_minimum_required(VERSION 3.17)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(baremetal-example)

add_library(eventrouter STATIC ../../eventrouter.c)
target_include_directories(eventrouter
  PUBLIC
  ../..
  .
)

add_subdirectory(../common_modules common_modules)

add_executable(main main.c)
target_link_libraries(main PRIVATE
  eventrouter
  module_data_logger
  module_data_uploader
  module_sensor_data_publisher
)
