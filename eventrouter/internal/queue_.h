#ifndef EVENTROUTER_QUEUE_H
#define EVENTROUTER_QUEUE_H

#include "event.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// An opaque queue type for ErEvent_t* elements.
    typedef void* ErQueue_t;
    typedef ErQueue_t ErQueueHandle_t;

    /// Allocates a new `ErQueue_t` that can hold at most `a_capacity` elements.
    ErQueue_t ErQueueNew(size_t a_capacity);

    /// Frees a `ErQueue_t` previously allocated with `ErQueueNew()`.
    void ErQueueFree(ErQueue_t a_queue);

    /// Blocks until there is a value to read from `a_queue`, then returns it.
    ErEvent_t* ErQueuePopFront(ErQueue_t a_queue);

    /// Blocks until there is space to write to the queue, then returns.
    void ErQueuePushBack(ErQueue_t a_queue, ErEvent_t* a_event);

    /// Returns true if `a_event` was read from `a_queue` within `a_ms`.
    bool ErQueueTimedPopFront(ErQueue_t a_queue, ErEvent_t** a_event,
                              int64_t a_ms);

    /// Returns true if `a_event` was written to `a_queue` within `a_ms`.
    bool ErQueueTimedPushBack(ErQueue_t a_queue, ErEvent_t* a_event,
                              int64_t a_ms);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_QUEUE_H */
