#ifndef EVENTROUTER_EVENTHANDLER_H
#define EVENTROUTER_EVENTHANDLER_H

#include "eventrouter/event.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// Values that `ErEventHandler_t` functions may return.
    typedef enum
    {
        /// The event handler did not expect to receive an event of this type;
        /// an error may have occurred, or an event was in transit when the
        /// module unsubscribed.
        ER_EVENT_HANDLER_RET__UNEXPECTED,

        /// The event handler is done with the event and it is safe to return it
        /// to its `m_sending_module`.
        ER_EVENT_HANDLER_RET__HANDLED,

        /// The event handler kept a reference to this event. Do not return this
        /// event to its sender. The module owning this event handler will
        /// return the event once it's done with it.
        ER_EVENT_HANDLER_RET__KEPT,
    } ErEventHandlerRet_t;

    /// A function which accepts events and returns qualitative information
    /// about how that event was received.
    typedef ErEventHandlerRet_t (*ErEventHandler_t)(ErEvent_t *a_event);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENTHANDLER_H */
