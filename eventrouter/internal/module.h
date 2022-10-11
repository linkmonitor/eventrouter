#ifndef EVENTROUTER_MODULE_H
#define EVENTROUTER_MODULE_H

#include <inttypes.h>
#include <limits.h>
#include <stddef.h>

#include "atomic.h"
#include "event_handler.h"
#include "event_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// Represents a unit of code which can send and receive events. Event
    /// Router modules are defined by their handler; multiple modules SHOULD NOT
    /// use the same handler unless the author takes appropriate steps.
    typedef struct
    {
        ErEventHandler_t m_handler;  /// Where events are delivered/returned.

        // Implementation details.
        size_t m_task_idx;
        size_t m_module_idx;
        uint8_t
            m_subscriptions[(ER_EVENT_TYPE__COUNT + (CHAR_BIT - 1)) / CHAR_BIT];
    } ErModule_t;

    /// Used to initialize `ErModule_t` definitions while avoiding
    /// missing-field-initializers warnings.
#define ER_CREATE_MODULE(a_handler)                                 \
    {                                                               \
        .m_handler = a_handler, .m_module_idx = 0, .m_task_idx = 0, \
        .m_subscriptions = {0},                                     \
    }

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_MODULE_H */
