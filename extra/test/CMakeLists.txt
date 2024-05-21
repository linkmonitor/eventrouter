add_executable(eventrouter_test
  common_eventrouter_test.cc
  $<$<IN_LIST:${IMPLEMENTATION},baremetal>:baremetal_eventrouter_test.cc>
  $<$<IN_LIST:${IMPLEMENTATION},freertos;posix>:os_eventrouter_test.cc>
  $<$<IN_LIST:${IMPLEMENTATION},freertos;posix>:mock_os.cc>
)
target_link_libraries(eventrouter_test PUBLIC
  eventrouter
  gtest_main
)
gtest_discover_tests(eventrouter_test)