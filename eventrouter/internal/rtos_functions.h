#ifndef EVENTROUTER_RTOS_FUNCTIONS_H
#define EVENTROUTER_RTOS_FUNCTIONS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        void (*SendEvent)(QueueHandle_t a_queue, void *a_event);
        TaskHandle_t (*GetCurrentTaskHandle)(void);
    } ErRtosFunctions_t;

    /// Overrides the RTOS functions used by this module. This makes testing
    /// easier and is not needed in device applications; default implementations
    /// for these functions are selected when this is not called.
    void ErSetRtosFunctions(const ErRtosFunctions_t *a_fns);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_RTOS_FUNCTIONS_H */
