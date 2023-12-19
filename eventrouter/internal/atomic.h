#ifndef EVENTROUTER_ATOMIC_H
#define EVENTROUTER_ATOMIC_H

/// This workaround is necessary to make the `atomic_*` definitions play nicely
/// in both C and C++ files.
#ifdef __cplusplus
#include <atomic>
using std::atomic_int;
using std::atomic_flag;
// Add more aliases here if necessary.
#else
#include <stdatomic.h>
#endif

/// Helper macro to initialize atomic_int
#ifdef __cplusplus
#define INIT_ATOMIC_INT(x) \
    {                      \
        (x)                \
    }
#else
#define INIT_ATOMIC_INT(x) (x)
#endif  // __cplusplus

#endif /* EVENTROUTER_ATOMIC_H */
