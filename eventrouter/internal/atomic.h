#ifndef EVENTROUTER_ATOMIC_H
#define EVENTROUTER_ATOMIC_H

/// This workaround is necessary to make the `atomic_*` definitions play nicely
/// in both C and C++ files.
#ifdef __cplusplus
#include <atomic>
#ifndef atomic_int
#define atomic_int std::atomic<int>
#endif
#ifndef atomic_flag
#define atomic_flag std::atomic_flag
#endif
// Add more aliases here if necessary.
#else
#include <stdatomic.h>
#endif

#endif /* EVENTROUTER_ATOMIC_H */
