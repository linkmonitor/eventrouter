#ifndef EVENTROUTER_TASK_H
#define EVENTROUTER_TASK_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "checked_config.h"

#ifdef ER_CONFIG_OS
#include "os_types.h"
#endif

#include "event_type.h"
#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /// Represents a task which participates in event routing.
    typedef struct
    {
        // OS-based implementations juggle multiple tasks and send events
        // between them; those responsibilities require more state.
#ifdef ER_CONFIG_OS
        /// Used to identify the task some event router functions are called in.
        ErTaskHandle_t m_task_handle;
        /// The queue that this task draws `ErEvent_t*` entries from.
        ErQueueHandle_t m_event_queue;
        /// A superset of module subscriptions within a task. This optimization
        /// makes task selection faster in `ErSend()` (and related functions).
        uint8_t
            m_subscriptions[(ER_EVENT_TYPE__COUNT + (CHAR_BIT - 1)) / CHAR_BIT];
#endif

        /// The list of modules this task contains; multiple tasks MUST NOT
        /// contain the same module. Each task MUST contain at least one module.
        ErModule_t **m_modules;
        size_t m_num_modules;
    } ErTask_t;

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_TASK_H */
