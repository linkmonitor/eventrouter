#ifndef EVENTROUTER_OS_FUNCTIONS_H
#define EVENTROUTER_OS_FUNCTIONS_H

#include "checked_config.h"

#ifndef ER_CONFIG_OS
#error "These functions only apply to OS-backed implementations."
#endif

#include "os_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// Operating system functions for which there is a safe default. Do not add
    /// functions to this struct if a safe default is not possible; consider
    /// promoting those functions to a field in `ErOptions_t` and add the
    /// appropriate checks in all implementations.
    typedef struct
    {
        void (*SendEvent)(ErQueueHandle_t a_queue, void *a_event);
        void (*ReceiveEvent)(ErQueueHandle_t a_queue, ErEvent_t **a_event);
        void (*TimedReceiveEvent)(ErQueueHandle_t a_queue, ErEvent_t **a_event,
                                  int64_t a_ms);
        ErTaskHandle_t (*GetCurrentTaskHandle)(void);
    } ErOsFunctions_t;

    /// Overrides the OS functions used by this module. This makes testing
    /// easier and is not needed in device applications; default implementations
    /// for these functions are selected when this is not called.
    void ErSetOsFunctions(const ErOsFunctions_t *a_fns);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_OS_FUNCTIONS_H */
