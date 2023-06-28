#ifndef EVENTROUTER_OS_TYPES_H
#define EVENTROUTER_OS_TYPES_H

#include "checked_config.h"

#ifndef ER_CONFIG_OS
#error "ER_CONFIG_OS must be defined to include this header"
#endif

#if ER_IMPLEMENTATION == ER_IMPL_FREERTOS
#include "os_types_freertos.h"
#elif ER_IMPLEMENTATION == ER_IMPL_POSIX
#include "os_types_posix.h"
#elif
#error "Unexpected value in ER_IMPLEMENTATION"
#endif

#endif /* EVENTROUTER_OS_TYPES_H */
