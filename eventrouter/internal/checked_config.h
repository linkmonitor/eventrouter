#ifndef EVENTROUTER_CHECKED_CONFIG_H
#define EVENTROUTER_CHECKED_CONFIG_H

/// This file includes the user-provided eventrouter_config.h, checks its
/// contents for errors, and provides defaults (as necessary).
#include "eventrouter_config.h"

//==============================================================================
// Utility definitions
//==============================================================================

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

//==============================================================================
// Implementation Selection
//==============================================================================

// These are the supported event router implementations. Clients select one by
// defining ER_IMPLEMENTATION to it. For example, placing the following
// definition in eventrouter_config.h chooses the FreeRTOS implementation:
//
//    #define ER_IMPLEMENTATION ER_IMPL_FREERTOS
//
#define ER_IMPL_FREERTOS  1
#define ER_IMPL_POSIX     2
#define ER_IMPL_BAREMETAL 3

// In previous versions of the event router, clients specified ER_FREERTOS and
// ER_BAREMETAL directly, instead of defining ER_IMPLEMENTATION. These checks
// convert old choices to new ones for backward compatibility. They also make it
// possible to specify an implementation from the command line; e.g., setting
// `-DER_IMPLEMENTATION=ER_FREERTOS` on the command line doesn't work.
#if defined(ER_FREERTOS) && defined(ER_BAREMETAL)
#error "Only one of ER_FREERTOS and ER_BAREMETAL may be defined"
#endif
#if defined(ER_FREERTOS)
#define ER_IMPLEMENTATION ER_IMPL_FREERTOS
#endif
#if defined(ER_POSIX)
#define ER_IMPLEMENTATION ER_IMPL_POSIX
#endif
#if defined(ER_BAREMETAL)
#define ER_IMPLEMENTATION ER_IMPL_BAREMETAL
#endif

// The FreeRTOS implementation is the default for historical reasons.
#define ER_IMPLEMENTATION_DEFAULT ER_IMPL_FREERTOS

// Make sure ER_IMPLEMENTATION is valid and provide a default if necessary.
#if !defined(ER_IMPLEMENTATION)
#define ER_IMPLEMENTATION ER_IMPLEMENTATION_DEFAULT
#elif 1 - ER_IMPLEMENTATION - 1 == 2  // Detect "#define ER_IMPLEMENTATION".
#error "ER_IMPLEMENTATION cannot be defined with no body."
#elif ER_IMPLEMENTATION == ER_IMPL_FREERTOS   // Valid selection.
#elif ER_IMPLEMENTATION == ER_IMPL_POSIX      // Valid selection.
#elif ER_IMPLEMENTATION == ER_IMPL_BAREMETAL  // Valid selection.
#else
#error "ER_IMPLEMENTATION has an invalid value."
#endif

//==============================================================================
// Configuration Calculation
// ==============================================================================
// Derived event router configurations, based on other configurations.

// Both the POSIX and FreeRTOS implementations require OS abstractions.
#if (ER_IMPLEMENTATION == ER_IMPL_FREERTOS) || \
    (ER_IMPLEMENTATION == ER_IMPL_POSIX)
#define ER_CONFIG_OS
#endif

//==============================================================================
// Core Configuration
//==============================================================================

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

#endif /* EVENTROUTER_CHECKED_CONFIG_H */
