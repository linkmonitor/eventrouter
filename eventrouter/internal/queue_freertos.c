#include "queue_.h"

#include <assert.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "event.h"

//==============================================================================
// Macros And Defines
//==============================================================================

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(a_time_ms)                                               \
    ((TickType_t)(((TickType_t)(a_time_ms) * (TickType_t)configTICK_RATE_HZ) / \
                  (TickType_t)1000U))
#endif

//==============================================================================
// Public Functions
//==============================================================================

ErQueue_t ErQueueNew(size_t a_capacity)
{
    return xQueueCreate(a_capacity, sizeof(ErEvent_t *));
}

void ErQueueFree(ErQueue_t a_queue)
{
    vQueueDelete(a_queue);
}

ErEvent_t *ErQueuePopFront(ErQueue_t a_queue)
{
    ErEvent_t *event = NULL;
    assert(pdTRUE == xQueueReceive(a_queue, &event, portMAX_DELAY));
    return event;
}

void ErQueuePushBack(ErQueue_t a_queue, ErEvent_t *a_event)
{
    assert(pdTRUE == xQueueSend(a_queue, &a_event, portMAX_DELAY));
}

bool ErQueueTimedPopFront(ErQueue_t a_queue, ErEvent_t **a_event, int64_t a_ms)
{
    BaseType_t ret = xQueueReceive(a_queue, a_event, pdMS_TO_TICKS(a_ms));
    return (ret == pdTRUE);
}

bool ErQueueTimedPushBack(ErQueue_t a_queue, ErEvent_t *a_event, int64_t a_ms)
{
    BaseType_t ret = xQueueSend(a_queue, &a_event, pdMS_TO_TICKS(a_ms));
    return (ret = pdTRUE);
}
