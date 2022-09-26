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

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "eventrouter/event.h"
#include "eventrouter/event_handler.h"
#include "eventrouter/module_id.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// Stores task attributes the Event Router is interested in.
    typedef struct
    {
        /// `ErInit_t` functions accept a task-ordered list of
        /// modules. The `m_first_module` and `m_last_module` are the IDs of the
        /// first and last entries in that list which should run in this task.
        /// See the relevant comments in `ErOptions_t` for more info.
        ErModuleId_t m_first_module;
        ErModuleId_t m_last_module;

        /// The queue that this task will draw `ErEvent_t*` entries from.
        QueueHandle_t m_event_queue;

        /// Used by some event router functions to check which task they are
        /// being called from.
        TaskHandle_t m_task_handle;
    } ErTaskDesc_t;

    /// These parameters define how an instance of the event router behaves. Any
    /// instance of this struct which is passed to a `ErInit_t`
    /// function MUST NOT be modified or freed once passed.
    typedef struct
    {
        /// These fields list all tasks that can participate in event routing.
        /// The tasks should be listed from highest priority to lowest. All
        /// tasks must contain at least one module and have a non-NULL queue.
        const ErTaskDesc_t *m_tasks;
        size_t m_num_tasks;

        /// These fields define a "task-ordered" list of non-NULL event
        /// handlers. This means that the event handlers in this list should be
        /// grouped according to which tasks "owns" them.
        ///
        ///            Task List           Event Handler List
        ///            ┌───────┐     ┌─────▶┌─────────────┐
        ///            │       │─────┤      ├─────────────┤
        ///            └───────┘     └─────▶└─────────────┘
        ///                          ┌─────▶┌─────────────┐
        ///            ┌───────┐     │      ├─────────────┤
        ///            │       │─────┤      ├─────────────┤
        ///            └───────┘     │      ├─────────────┤
        ///                          └─────▶└─────────────┘
        ///            ┌───────┐     ┌─────▶┌─────────────┐
        ///            │       │─────┤      ├─────────────┤
        ///            └───────┘     └─────▶└─────────────┘
        ///
        /// Said differently, the first N event handlers belong to Task 1. The
        /// next M event handlers belong to Task 2, and so on.
        const ErEventHandler_t *m_event_handlers;
        size_t m_num_event_handlers;

        /// The values for all routable event types must form an integer range
        /// without gaps. E.g. if there are 20 event types and they start at 17,
        /// then every number from 17 up-to-and-including 36 must be a valid
        /// event type.
        size_t m_first_event_type;
        size_t m_num_event_types;

        /// System-specific functions which the Event Router needs to function.
        struct
        {
            /// Returns true if the current execution context is an interrupt
            /// service routine. This field is required.
            bool (*m_IsInIsr)(void);

            /// Provides diagnostic information in case of a failure. This field
            /// is optional and may be left NULL.
            void (*m_LogError)(const char *a_format, ...);

            /// These functions should behave like their standard library
            /// counterparts if specified. If omitted (set to NULL) the standard
            /// library counterpart will take their place.
            void *(*m_Malloc)(size_t a_size);
            void (*m_Free)(void *a_block);
        } m_system_funcs;

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
    /// This function MUST be called from the task that owns the module with
    /// `a_id`; it MUST NOT be called from an interrupt or a callback.
    void ErSubscribe(ErModuleId_t a_id, ErEventType_t a_event_type);

    /// Prevents the event router from delivering any events of `a_event_type`
    /// to this module's event handler.
    ///
    /// This function MUST be called from the task that owns the module with
    /// `a_id`; it MUST NOT be called from an interrupt or a callback.
    void ErUnsubscribe(ErModuleId_t a_id, ErEventType_t a_event_type);

#ifdef __cplusplus
}
#endif

#endif /* EVENTROUTER_EVENTROUTER_H */
