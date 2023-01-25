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

/// Extensions only available in the baremetal implementation.
#ifdef ER_BAREMETAL
#include "eventrouter/internal/eventrouter_baremetal.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /// These parameters define how an instance of the event router behaves. Any
    /// instance of this struct which is passed to a `ErInit_t`
    /// function MUST NOT be modified or freed once passed.
    typedef struct
    {
        /// These fields list all tasks that can participate in event routing.
        /// The tasks should be listed from highest priority to lowest.
        ErTask_t *m_tasks;
        size_t m_num_tasks;

        /// Returns true if the current execution context is an interrupt
        /// service routine.
        bool (*m_IsInIsr)(void);
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

    /// Returns an event which a module previously KEPT.
    ///
    /// Modules KEEP events by returning `ER_EVENT_HANDLER_RET__KEPT` from their
    /// event handler. Keeping an event lets a module inspect the contents of
    /// that event across multiple calls to their event handler. Normally,
    /// modules lose access to an event after their event handler returns.
    ///
    /// When modules are done with an event they have previously KEPT, they must
    /// call `ErReturnToSender()` on that event. This lets the sender reclaim
    /// the resources for that event and reuse it. Modules MUST NOT call
    /// `ErReturnToSender()` in the same handler call that they return
    /// `ER_EVENT_HANDLER_RET__KEPT`; it corrupts the reference count achieves
    /// the same thing as returning `ER_EVENT_HANDLER_RET__HANDLED` instead.
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

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENTROUTER_H */
