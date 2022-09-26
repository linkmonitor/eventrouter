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
