#ifndef EVENTROUTER_TYPE_H
#define EVENTROUTER_TYPE_H

#include "checked_config.h"

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

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_TYPE_H */
