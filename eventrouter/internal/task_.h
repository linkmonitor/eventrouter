#ifndef EVENTROUTER_TASK_H
#define EVENTROUTER_TASK_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "checked_config.h"

#ifdef ER_FREERTOS
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#endif  // ER_FREERTOS

#include "event_type.h"
#include "module.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /// Represents a FreeRTOS task which participates in event routing.
    typedef struct
    {
#ifdef ER_FREERTOS
        /// Used to identify the task some event router functions are called in.
        TaskHandle_t m_task_handle;
        /// The queue that this task draws `ErEvent_t*` entries from.
        QueueHandle_t m_event_queue;
        /// A superset of module subscriptions within a task. This optimization
        /// makes task selection faster in `ErSend()` (and related functions).
        uint8_t
            m_subscriptions[(ER_EVENT_TYPE__COUNT + (CHAR_BIT - 1)) / CHAR_BIT];
#endif  // ER_FREERTOS

        /// The list of modules this task contains; multiple tasks MUST NOT
        /// contain the same module. Each task MUST contain at least one module.
        ErModule_t **m_modules;
        size_t m_num_modules;
    } ErTask_t;

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_TASK_H */
