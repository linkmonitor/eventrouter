#include "eventrouter/internal/checked_config.h"

#if defined(ER_FREERTOS)
#include "eventrouter/internal/eventrouter_freertos.c"
#elif defined(ER_BAREMETAL)
#include "eventrouter/internal/eventrouter_baremetal.c"
#include "eventrouter/internal/list.c"
#elif defined(ER_POSIX)
#include "eventrouter/internal/eventrouter_posix.c"
#else
#error "Unsupported implementation."
#endif
