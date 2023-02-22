#===============================================================================
# Common Modules
# ===============================================================================

set(COMMON_MODULE_SOURCES # Related header files are directory-local.
  module_data_logger.c
  module_data_uploader.c
  module_sensor_data_publisher.c
)

#===============================================================================
# Baremetal
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
