#===============================================================================
# Common Modules
#
# These modules are common to all example applications to demonstrate the
# reusability of event-based code across eventrouter implementations.
# ===============================================================================

set(COMMON_MODULE_SOURCES # Related header files are directory-local.
  module_data_logger.c
  module_data_uploader.c
  module_sensor_data_publisher.c
)

#===============================================================================
# Baremetal: The simplest example application.
# ===============================================================================

add_executable(baremetal_eventrouter_example
  ${COMMON_MODULE_SOURCES}
  baremetal_eventrouter_example.c
)
target_link_libraries(baremetal_eventrouter_example
  PRIVATE
  baremetal_eventrouter
)

#===============================================================================
# FreeRTOS
# ===============================================================================

add_executable(freertos_eventrouter_example
  ${COMMON_MODULE_SOURCES}
  freertos_eventrouter_example.c
  task_generic.c
)
target_link_libraries(freertos_eventrouter_example
  PRIVATE
  freertos_eventrouter
)

#===============================================================================
# POSIX
# ===============================================================================

add_executable(posix_eventrouter_example
  ${COMMON_MODULE_SOURCES}
  posix_eventrouter_example.c
  task_generic.c
)
target_link_libraries(posix_eventrouter_example
  PRIVATE
  posix_eventrouter
)
