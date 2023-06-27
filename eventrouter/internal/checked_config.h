#ifndef EVENTROUTER_CHECKED_CONFIG_H
#define EVENTROUTER_CHECKED_CONFIG_H

/// This file includes the user-provided eventrouter_config.h, checks its
/// contents for errors, and provides defaults (as necessary).
#include "eventrouter_config.h"

/// Clients may provide their own assertion macro. If they do not, the Event
/// Router defaults to C's standard `assert()` macro.
#ifndef ER_ASSERT
#include <assert.h>
#define ER_ASSERT(condition) assert(condition)
#endif

/// Clients may provide their own static assertion macro. If they do not, the
/// Event Router defaults to a language-dependent default.
#ifndef ER_STATIC_ASSERT
#include <assert.h>
#if defined(static_assert) || defined(__cplusplus)
#define ER_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#if !defined(CAT) && !defined(CAT2)
#define CAT2(x, y) x##y
#define CAT(x, y)  CAT2(x, y)
#endif
#define ER_STATIC_ASSERT(cond, msg)                       \
    enum                                                  \
    {                                                     \
        CAT(STATIC_ASSERT_, __COUNTER__) = 1 / (!!(cond)) \
    }
#endif
#endif

/// Default to the FreeRTOS implementation. Clients may specify ER_BAREMETAL in
/// their eventrouter_config.h and then build without FreeRTOS.
#if !defined(ER_FREERTOS) && !defined(ER_BAREMETAL)
#define ER_FREERTOS
#endif

/// Sanity check the implementation definitions.
#if defined(ER_FREERTOS) && defined(ER_BAREMETAL)
#error "Only one of ER_FREERTOS and ER_BAREMETAL may be defined"
#endif

/// All values in `ErEventType_t` must be monotonically increasing without gaps
/// but they do not have to start at 0. ER_EVENT_TYPE__OFFSET specifies the
/// value of the first entry in `ER_EVENT_TYPE__ENTRIES` below.
#ifndef ER_EVENT_TYPE__OFFSET
#define ER_EVENT_TYPE__OFFSET 0
#endif

/// `ER_EVENT_TYPE__ENTRIES` must be an X-macro that defines a list of entries.
/// The following is an example of a valid definition:
///
///     // NOTE: There should be backslashes at the end of these lines but it
///     // triggers a warning about multi-line comments.
///     #define ER_EVENT_TYPE__ENTRIES
///         X(ER_EVENT_TYPE__A)
///         X(ER_EVENT_TYPE__B)
///         X(ER_EVENT_TYPE__C)
///
/// Clients MUST NOT define entries with hardcoded integer values, like
/// X(ER_EVENT_TYPE__A=10), since it may introduce aliasing that will break the
/// event router and will (more than likely) cause the event router to allocate
/// more memory than necessary.
#ifndef ER_EVENT_TYPE__ENTRIES
#error "ER_EVENT_TYPE__ENTRIES must be defined."
#endif

/// Specifies the name of the `ErEvent_t` member in types which derive from
/// `ErEvent_t`. This macro powers the `MIXIN_ER_EVENT`, `TO_ER_EVENT()`, and
/// `FROM_ER_EVENT()` macros. Clients should define this value if name
/// conflicts occur in derived types.
#ifndef ER_EVENT_MEMBER
#define ER_EVENT_MEMBER m_event
#endif

/// Returns the address of the structure containing the member.
#ifndef container_of
#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })
#endif

#endif /* EVENTROUTER_CHECKED_CONFIG_H */
