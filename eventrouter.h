#ifndef EVENTROUTER_EVENTROUTER_H
#define EVENTROUTER_EVENTROUTER_H

/// The Event Router is responsible for transporting events sent by one module
/// to all modules which are interested in events of that type, and then
/// returning that event to the sending module.
///
/// The Event Router accomplishes this by maintaining a map between:
///
///                 Event Types -> Modules -> Tasks -> Queues
///
/// The mapping is defined in an `ErOptions_t` struct and set during
/// initialization; it MUST NOT change once set.
///
/// The Event Router tracks each module's interest in specific event types using
/// a subscription metaphor. Modules can subscribe to event types or unsubscribe
/// from them at any time using `ErSubscribe()` and its counterpart.

#include "eventrouter/internal/event.h"
#include "eventrouter/internal/module.h"
#include "eventrouter/internal/task_.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //============================================================================
    // Functions available in all implementations.
    //============================================================================

    /// These parameters define how an instance of the event router behaves. Any
    /// instance of this struct which is passed to a `ErInit_t` function MUST
    /// NOT be modified or freed once passed.
    typedef struct
    {
        /// These fields list all tasks that can participate in event routing.
        /// The tasks should be listed from highest priority to lowest.
        const ErTask_t *m_tasks;
        size_t m_num_tasks;

#ifdef ER_CONFIG_OS
        /// Returns true if the current execution context is an interrupt
        /// service routine.
        bool (*m_IsInIsr)(void);
#endif
    } ErOptions_t;

    /// Initializes the event router based on the options provided and must be
    /// called before calling any other event router functions. Once called,
    /// this cannot be called again until `ErDeinit()` is called.
    void ErInit(const ErOptions_t *a_options);

    /// Undoes all actions taken by `ErInit()`. This function asserts
    /// if called when uninitialized or de-initialized.
    void ErDeinit(void);

    /// Delivers a copy of `a_event` to all modules which subscribe to this
    /// event's type and then returns the event to the sending module.
    ///
    /// This function asserts if `a_event` is NULL, its type is outside the
    /// range specified by `ErOptions_t` during initialization, or if
    /// it is called on an event which has already been sent and has not yet
    /// been returned (this can be checked with `ErEventIsInFlight()`).
    ///
    /// This function is safe to call from within a task or an interrupt.
    void ErSend(ErEvent_t *a_event);

    /// Customizes the behavior of `ErSendExtended()`.
    typedef struct
    {
        /// NOTE: Only supported in the FreeRTOS implementation.
        ///
        /// Permits re-sending an event that is already in flight (check with
        /// `ErEventIsInFlight()`). All subscribers receive the event one time
        /// for each time the event is sent and re-sent. The sending module
        /// receives the event *once* after all subscribers are done with it;
        /// the sender can still use re-sent events as proxies for ownership.
        ///
        /// When true, all calls to `ErSendEx()` must either occur in
        /// the task that owns `a_event->m_sending_module` or in an interrupt;
        /// the implementation checks this and will assert if violated.
        bool m_allow_resending;
    } ErSendExOptions_t;

    /// Delivers a copy of `a_event` to all modules which subscribe to this
    /// event's type and then returns the event to the sending module.
    ///
    /// This function is similar to `ErSend()` but more flexible (the
    /// 'Ex' in the function name stands for 'Extended'); it allows clients to
    /// trade some of `ErSend()`s restrictions for other limitations
    /// that the client may prefer.
    ///
    /// Read the documentation for `ErSendExOptions_t` carefully to
    /// understand both the new features and the new limitations.
    void ErSendEx(ErEvent_t *a_event, ErSendExOptions_t a_options);

    /// Delivers the event to the `ErEventHandler_t` of all modules in the
    /// current task which should receive an event of this type.
    ///
    /// Call this function on an event exactly once every time it is
    /// received from a queue. Calling it more or less than that is an
    /// error.
    void ErCallHandlers(ErEvent_t *a_event);

    /// If an event handler "keeps" and event, indicated by returning
    /// `ER_EVENT_HANDLER_RET__KEPT`, they are responsible for calling this
    /// function on the event when they are done with it. Said differently, the
    /// event handler must call this function once if and only if it returns
    /// `ER_EVENT_HANDLER_RET__KEPT`.
    void ErReturnToSender(ErEvent_t *a_event);

    /// Causes the event router to deliver all events of `a_event_type` to this
    /// module's event handler. Modules are not subscribed to any event types
    /// when the event router is initialized.
    ///
    /// This function MUST be called from the task that owns `a_module`; it MUST
    /// NOT be called from an interrupt or a callback.
    void ErSubscribe(ErModule_t *a_module, ErEventType_t a_event_type);

    /// Prevents the event router from delivering any events of `a_event_type`
    /// to this module's event handler.
    ///
    /// This function MUST be called from the task that owns `a_module`; it MUST
    /// NOT be called from an interrupt or a callback.
    void ErUnsubscribe(ErModule_t *a_module, ErEventType_t a_event_type);

    //============================================================================
    // Implementation-Specific Functions
    //============================================================================

#ifdef ER_CONFIG_OS
    /// Returns true if the caller successfully claimed the event and false
    /// otherwise. Callers in non-owning tasks must claim events before sending
    /// them with `ErSend()` or `ErSendEx()`; the implementation checks this and
    /// asserts if violated. For more information see "Claiming Events" section
    /// at the top of this file.
    bool ErTryClaim(ErEvent_t *a_event);

    /// Blocks until the next event sent to the current task is received, and
    /// returns it; asserts if called from an interrupt.
    ErEvent_t *ErReceive(void);

    /// Blocks until either the next event sent to the current task is received
    /// or `a_ms` milliseconds have passed. This function returns NULL on
    /// timeout and asserts if called from an interrupt.
    ErEvent_t *ErTimedReceive(int64_t a_ms);

#elif ER_IMPLEMENTATION == ER_IMPL_BAREMETAL
    /// Must be called at the beginning of a new event loop.
    void ErNewLoop(void);

    /// Returns events that are scheduled for delivery this loop. Clients should
    /// call this in a loop until it returns NULL and pass all non-NULL events
    /// to `ErCallHandlers()`. Events which are not delivered this loop will be
    /// delivered on the next loop (or whenever they first get a chance).
    ///
    /// NOTE: It's tempting to combine this function with `ErReceive()` because
    /// it has an identical signature, but it has different semantics; baremetal
    /// functions are not allowed to block, and this should be called in a loop.
    ErEvent_t *ErGetEventToDeliver(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENTROUTER_H */
