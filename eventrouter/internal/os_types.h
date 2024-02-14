#ifndef EVENTROUTER_OS_TYPES_H
#define EVENTROUTER_OS_TYPES_H

/// @file This file provides implementation-specific types for tasks and queues,
/// and the header files necessary for making sense of them.

#include "checked_config.h"

#ifndef ER_CONFIG_OS
#error "ER_CONFIG_OS must be defined to include this header"
#endif

#include "queue_.h"

#if ER_IMPLEMENTATION == ER_IMPL_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#define ErTaskHandle_t TaskHandle_t
#elif ER_IMPLEMENTATION == ER_IMPL_POSIX
#include <pthread.h>
#define ErTaskHandle_t pthread_t
#else
#error "Unexpected value in ER_IMPLEMENTATION"
#endif

#endif /* EVENTROUTER_OS_TYPES_H */
