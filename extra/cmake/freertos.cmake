include(FetchContent)

FetchContent_Declare(
  freertos
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel
  GIT_TAG V10.5.1
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/FreeRTOSConfig.h
[[
#ifndef FREERTOSCONFIG_H
#define FREERTOSCONFIG_H

#include <stdbool.h>

/// These values come from starting with an empty file and silencing compiler
/// errors by providing reasonable (?) values. These specific values are not
/// necessary for the Event Router to function.

#define configMAX_PRIORITIES         10
#define configMINIMAL_STACK_SIZE     1024
#define configTICK_RATE_HZ           1000
#define configTOTAL_HEAP_SIZE        (1024 * 1024)
#define configUSE_16_BIT_TICKS       false
#define configUSE_IDLE_HOOK          false
#define configUSE_PREEMPTION         true
#define configUSE_TICK_HOOK          false
#define configUSE_TIMERS             true
#define configTIMER_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE
#define configTIMER_TASK_PRIORITY    3
#define configTIMER_QUEUE_LENGTH     10

#endif /* FREERTOSCONFIG_H */
]]
)

set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "")
set(FREERTOS_HEAP 4 CACHE STRING "")
set(FREERTOS_PORT GCC_POSIX CACHE STRING "")

FetchContent_MakeAvailable(freertos)
target_link_options(freertos_kernel PUBLIC
  $<$<PLATFORM_ID:Linux>:-pthread>
)
