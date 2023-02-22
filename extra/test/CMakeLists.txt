#===============================================================================
# Baremetal
# ===============================================================================

add_executable(baremetal_eventrouter_test
  baremetal_eventrouter_test.cc
  common_eventrouter_test.cc
  mock_options.cc
)
target_link_libraries(baremetal_eventrouter_test PRIVATE
  baremetal_eventrouter
  gtest_main
)
gtest_discover_tests(baremetal_eventrouter_test)

#===============================================================================
# FreeRTOS
# ===============================================================================

add_executable(freertos_eventrouter_test
  common_eventrouter_test.cc
  # TODO(jjaoudi): Add freertos_eventrouter_test.cc
  mock_options.cc
)
target_link_libraries(freertos_eventrouter_test PRIVATE
  freertos_eventrouter
  gtest_main
)
gtest_discover_tests(freertos_eventrouter_test)
