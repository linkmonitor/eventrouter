#include "eventrouter/internal/checked_config.h"

#ifdef ER_CONFIG_OS
#include "eventrouter/internal/eventrouter_freertos.c"
#elif ER_IMPLEMENTATION == ER_IMPL_BAREMETAL
#include "eventrouter/internal/eventrouter_baremetal.c"
#include "eventrouter/internal/list.c"
#else
#error "Unsupported implementation."
#endif
