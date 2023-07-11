#ifndef EVENTROUTER_ATOM_LOCK_H
#define EVENTROUTER_ATOM_LOCK_H

#include <stdbool.h>

#include "atomic.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// A lock-like abstraction that doesn't support blocking. Clients may try
    /// to take the lock or release it once taken. This type of lock *does not*
    /// involve the operating system so trying to take it is cheap and it can be
    /// used safely in interrupt contexts. Since this lock does not block it can
    /// be used with locks that block without introducing a deadlock; these
    /// locks don't prevent deadlocks, but they can't add them either.
    typedef atomic_flag ErAtomLock_t;

    /// Returns true if the lock was successfully taken and false otherwise.
    /// Asserts if `a_lock` is `NULL`.
    __attribute__((unused)) static inline bool ErAtomLockTryTake(
        volatile ErAtomLock_t *a_lock)
    {
        return atomic_flag_test_and_set(a_lock) == 0;
    }

    /// Releases the lock if taken and does nothing otherwise. Asserts if
    /// `a_lock` is `NULL`.
    __attribute__((unused)) static inline void ErAtomLockGive(
        ErAtomLock_t *a_lock)
    {
        atomic_flag_clear(a_lock);
    }

#define ER_ATOM_LOCK_INIT ATOMIC_FLAG_INIT

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_ATOM_LOCK_H */
