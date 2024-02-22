#ifndef EVENTROUTER_EVENT_H
#define EVENTROUTER_EVENT_H

#include <stdbool.h>

#include "atomic.h"
#include "defs.h"
#include "event_type.h"
#include "module.h"

#ifdef ER_CONFIG_OS
#include "atom_lock.h"
#endif

#if ER_IMPLEMENTATION == ER_IMPL_BAREMETAL
#include "list.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /// Contains the fields the Event Router needs to route events and manage
    /// subscriptions. New event structures must contain an `ErEvent_t` member
    /// whose name matches the expansion of `ER_EVENT_MEMBER`. This is most
    /// easily accomplished using `MIXIN_ER_EVENT`.
    typedef struct ErEvent_t
    {
        ErEventType_t m_type;
        atomic_int m_reference_count;
        ErModule_t *m_sending_module;
#ifdef ER_CONFIG_OS
        ErAtomLock_t m_lock;
#elif ER_IMPLEMENTATION == ER_IMPL_BAREMETAL
        ErList_t m_next;
#endif
    } ErEvent_t;

    /// Returns true if the event is in the process of being delivered to
    /// subscribers or returned to the sending module.
    static inline bool ErEventIsInFlight(const ErEvent_t *a_event)
    {
        return atomic_load(&a_event->m_reference_count) != 0;
    }

    /// Initializes an `ErEvent_t` struct.
    static inline void ErEventInit(ErEvent_t *a_event, ErEventType_t a_type,
                                   ErModule_t *a_module)
    {
        a_event->m_type            = a_type;
        a_event->m_reference_count = 0;
        a_event->m_sending_module  = a_module;
#ifdef ER_CONFIG_OS
        atomic_flag_clear(&a_event->m_lock);
#elif ER_IMPLEMENTATION == ER_IMPL_BAREMETAL
        a_event->m_next.m_next = NULL;
#endif
    }

    /// Add this to the top-level of a struct definition to make values of that
    /// type compatible with the Event Router (assuming the use of the helper
    /// macros below). Valid uses of this macro look like:
    ///
    ///     typedef struct {
    ///         MIXIN_ER_EVENT;
    ///         int m_value;
    ///         const char *m_phrase;
    ///     } Foo_t;
    ///
    ///     typedef struct {
    ///         int m_value;
    ///         MIXIN_ER_EVENT;
    ///         const char *m_phrase;
    ///     } Bar_t;
    ///
    /// Note that `MIXIN_ER_EVENT` does NOT need to come first, but that placing
    /// it first can make the code generated for `TO_ER_EVENT()` and
    /// `FROM_ER_EVENT()` (marginally) more efficient.
#define MIXIN_ER_EVENT \
    ErEvent_t ER_EVENT_MEMBER  // Parentheses and semicolon
                               // omitted intentionally.

    /// Returns a pointer to the `ErEvent_t` member of `a_struct` type that
    /// includes `MIXIN_ER_EVENT`; `a_struct` must be a value and not a pointer.
#define TO_ER_EVENT(a_struct) (&(a_struct).ER_EVENT_MEMBER)

    /// Returns the struct of `a_type` that includes the member pointed to by
    /// `a_event_p`. The developer must ensure that `a_type` is correct. This
    /// macro returns the surrounding struct by value instead of by reference.
#define FROM_ER_EVENT(a_event_p, a_type) \
    (*er_container_of(a_event_p, a_type, ER_EVENT_MEMBER))

    /// Initialize the event fields of a struct which mixes-in `ErEvent_t`
    /// behavior. This differs from `ErEventInit_t` in that it can be used in
    /// static definitions using designated initializers.
#if ER_IMPLEMENTATION == ER_IMPL_BAREMETAL
#define INIT_ER_EVENT(a_type, a_module)                          \
    .ER_EVENT_MEMBER = {.m_type            = a_type,             \
                        .m_reference_count = INIT_ATOMIC_INT(0), \
                        .m_sending_module  = a_module,           \
                        .m_next = { .m_next = NULL },            \
    }
#else /* ER_IMPLEMENTATION != ER_IMPL_BAREMETAL */
#define INIT_ER_EVENT(a_type, a_module)                          \
    .ER_EVENT_MEMBER = {.m_type            = a_type,             \
                        .m_reference_count = INIT_ATOMIC_INT(0), \
                        .m_sending_module  = a_module,           \
                        .m_lock            = ER_ATOM_LOCK_INIT}
#endif

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENT_H */
