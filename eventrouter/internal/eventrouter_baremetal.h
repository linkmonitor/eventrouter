#ifndef EVENTROUTER_EVENTROUTER_BAREMETAL_H
#define EVENTROUTER_EVENTROUTER_BAREMETAL_H

#include "event.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// Must be called at the beginning of a new event loop.
    void ErNewLoop(void);

    /// Returns events that are scheduled for deliver this loop. Clients should
    /// call this in a loop until it returns NULL and pass all non-NULL events
    /// to `ErCallHandlers()`. Events which are not delivered this loop will be
    /// delivered on the next loop (or whenever they first get a chance).
    ErEvent_t *ErGetEventToDeliver(void);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENTROUTER_BAREMETAL_H */
