#include "checked_config.h"

#if ER_IMPLEMENTATION == ER_IMPL_FREERTOS
#include "queue_freertos.c"
#elif ER_IMPLEMENTATION == ER_IMPL_POSIX
#include "queue_posix.c"
#else
#error "No queue implementation found."
#endif
