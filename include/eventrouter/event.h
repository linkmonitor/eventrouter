#ifndef EVENTROUTER_EVENT_H
#define EVENTROUTER_EVENT_H

#include <stdatomic.h>
#include <stdbool.h>

#include "eventrouter/checked_config.h"
#include "eventrouter/module_id.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // clang-format off
    typedef enum
    {
        /// This entry denotes an invalid event AND makes the first entry in
        /// ER_EVENT_TYPE__ENTRIES() equal ER_EVENT_TYPE__OFFSET.
        ER_EVENT_TYPE__INVALID = (ER_EVENT_TYPE__OFFSET - 1),

#define X(entry) entry,
        ER_EVENT_TYPE__ENTRIES
#undef X

        /// This entry provides a value from which we can derive *__COUNT and
        /// *__LAST below; it follows the last user-provided entry.
        ER_EVENT_TYPE__SENTINEL,
        ER_EVENT_TYPE__COUNT = ER_EVENT_TYPE__SENTINEL - ER_EVENT_TYPE__OFFSET,
        ER_EVENT_TYPE__FIRST = ER_EVENT_TYPE__OFFSET,
        ER_EVENT_TYPE__LAST  = ER_EVENT_TYPE__SENTINEL - 1,
    } ErEventType_t;
    // clang-format on

    /// Contains the fields the Event Router needs to route events and manage
    /// subscriptions. New event structures must contain an `ErEvent_t` member
    /// whose name matches the expansion of `ER_EVENT_MEMBER`. This is most
    /// easily accomplished using `MIXIN_ER_EVENT`.
    typedef struct
    {
        ErEventType_t m_type;
        atomic_int m_reference_count;
        ErModuleId_t m_sending_module;
    } ErEvent_t;

    /// Returns true if the event is in the process of being delivered to
    /// subscribers or returned to the sending module.
    static inline bool ErEventIsInFlight(const ErEvent_t *a_event)
    {
        return atomic_load(&a_event->m_reference_count) != 0;
    }

    /// Initializes an `ErEvent_t` struct.
    static inline void ErEventInit(ErEvent_t *a_event, ErEventType_t a_type,
                                   ErModuleId_t a_id)
    {
        a_event->m_type            = a_type;
        a_event->m_reference_count = 0;
        a_event->m_sending_module  = a_id;
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
    (*container_of(a_event_p, a_type, ER_EVENT_MEMBER))

    /// Initialize the event fields of a struct which mixes-in `ErEvent_t`
    /// behavior. This differs from `ErEventInit_t` in that it can be used in
    /// static definitions using designated initializers.
#define INIT_ER_EVENT(a_type, a_id) \
    .ER_EVENT_MEMBER = {.m_type = a_type, .m_sending_module = a_id}

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENT_H */
