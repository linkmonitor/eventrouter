#include "eventrouter.h"

#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bitref.h"
#include "defs.h"
#include "os_functions.h"

/// The maximum number of tasks the Event Router can support without changing
/// the dispatch strategy in `ErSend()`.
#define TASK_SEND_LIMIT (32)

//==============================================================================
// Static Variables
//==============================================================================

static struct
{
    bool m_initialized;
    const ErOptions_t *m_options;
    ErOsFunctions_t m_os_functions;
} s_context;

//==============================================================================
// Implementation-Specific Functions.
//==============================================================================

#if ER_IMPLEMENTATION == ER_IMPL_FREERTOS
static bool IsInIsr(void);  // Forward declaration.

static void DefaultSendEvent(ErQueueHandle_t a_queue, void *a_event)
{
    if (IsInIsr())
    {
        // The logic for detecting a higher-priority task and yielding to it is
        // taken directly the documentation for `xQueueSendToBackFromISR()`.
        BaseType_t higher_priority_task_was_woken = pdFALSE;
        ER_ASSERT(pdTRUE ==
                  xQueueSendToBackFromISR(a_queue, &a_event,
                                          &higher_priority_task_was_woken));

        if (higher_priority_task_was_woken)
        {
            taskYIELD();
        }
    }
    else
    {
        // TODO(jjaoudi): Replace this with a function that prints a
        // diagnostic when posting to full queue in debug builds.
        ER_ASSERT(pdTRUE == xQueueSendToBack(a_queue, &a_event, portMAX_DELAY));
    }
}

static void DefaultReceiveEvent(ErQueueHandle_t a_queue, ErEvent_t **a_event)
{
    ER_ASSERT(pdTRUE == xQueueReceive(a_queue, a_event, portMAX_DELAY));
}

static bool DefaultTimedReceiveEvent(ErQueueHandle_t a_queue,
                                     ErEvent_t **a_event, int64_t a_ms)
{
    return (pdTRUE == xQueueReceive(a_queue, a_event, pdMS_TO_TICKS(a_ms)));
}

static ErTaskHandle_t DefaultGetCurrentTaskHandle(void)
{
    return xTaskGetCurrentTaskHandle();
}
#elif ER_IMPLEMENTATION == ER_IMPL_POSIX
/* TODO(jjaoudi): Implement these appropriately.*/
#else
#error "Unsupported ER_IMPLEMENTATION selected."
#endif

//==============================================================================
// Core, OS-Agnostic Implementation
//==============================================================================

/// Returns true if the system is inside an interrupt handler.
static bool IsInIsr(void)
{
    ER_ASSERT(s_context.m_initialized);
    return s_context.m_options->m_IsInIsr();
}

/// Asserts if the contents of the `ErOptions_t` struct are invalid and
/// populates modules with values to make future lookups faster.
static void ValidateAndInitializeOptions(const ErOptions_t *a_options)
{
    ER_ASSERT(a_options != NULL);
    ER_ASSERT(a_options->m_tasks != NULL);
    ER_ASSERT(a_options->m_num_tasks > 0);
    ER_ASSERT(a_options->m_IsInIsr != NULL);
    ER_ASSERT(a_options->m_GetTimeMs != NULL);

    ER_STATIC_ASSERT(ER_EVENT_TYPE__COUNT > 0,
                     "There must be at least one type to route");

    // See the comment in `ErSend()` for more info.
    ER_ASSERT(a_options->m_num_tasks <= TASK_SEND_LIMIT);

    for (size_t task_idx = 0; task_idx < a_options->m_num_tasks; ++task_idx)
    {
        const ErTask_t *task = &a_options->m_tasks[task_idx];

        ER_ASSERT(task->m_task_handle != NULL);
        ER_ASSERT(task->m_event_queue != NULL);
        ER_ASSERT(task->m_modules != NULL);
        ER_ASSERT(task->m_num_modules > 0);

        for (size_t module_idx = 0; module_idx < task->m_num_modules;
             ++module_idx)
        {
            ErModule_t *module = task->m_modules[module_idx];

            ER_ASSERT(module->m_handler != NULL);

            module->m_task_idx   = task_idx;
            module->m_module_idx = module_idx;
            memset(&module->m_subscriptions, 0,
                   sizeof(module->m_subscriptions));
        }
    }
}

/// Returns true if this type is in the range set at initialization. This
/// function must be called after initialization completes.
static bool IsEventTypeRoutable(ErEventType_t a_type)
{
    return ((a_type >= ER_EVENT_TYPE__FIRST) &&
            (a_type <= ER_EVENT_TYPE__LAST));
}

/// Returns true if this module is owned by a task known to the Event Router.
/// This function must be called after initialization completes.
static bool IsModuleOwned(const ErModule_t *a_module)
{
    const ErModule_t *claimed_module =
        s_context.m_options->m_tasks[a_module->m_task_idx]
            .m_modules[a_module->m_module_idx];
    return claimed_module == a_module;
}

/// Returns true if this event can be delivered to subscribers and returned.
/// This function must be called after initialization completes.
static bool IsEventSendable(ErEvent_t *a_event)
{
    return IsEventTypeRoutable(a_event->m_type) &&
           IsModuleOwned(a_event->m_sending_module);
}

/// Returns the index of the task in `s_context.m_options->m_tasks` that
/// corresponds with the currently running task.
static size_t GetIndexOfCurrentTask(void)
{
    const ErOptions_t *options = s_context.m_options;
    const TaskHandle_t current_task =
        s_context.m_os_functions.GetCurrentTaskHandle();

    int task_idx = -1;
    for (size_t idx = 0; idx < options->m_num_tasks; ++idx)
    {
        if (current_task == options->m_tasks[idx].m_task_handle)
        {
            task_idx = idx;
            break;
        }
    }

    if (task_idx == -1)
    {
        ER_ASSERT(!"Task not registered with the event router");
    }

    return task_idx;
}

/// Events may only be re-sent if re-sending is explicitly allowed and the
/// sender is either in an interrupt or the sending module's task.
static bool EventResendingAllowed(const ErSendExOptions_t *a_options,
                                  size_t a_sending_task_idx)
{
    return a_options->m_allow_resending &&
           (IsInIsr() || (a_sending_task_idx == GetIndexOfCurrentTask()));
}

void ErInit(const ErOptions_t *a_options)
{
    ER_ASSERT(!s_context.m_initialized);
    ValidateAndInitializeOptions(a_options);
    s_context.m_options      = a_options;
    s_context.m_os_functions = (ErOsFunctions_t){
        .SendEvent            = DefaultSendEvent,
        .GetCurrentTaskHandle = DefaultGetCurrentTaskHandle,
    };
    s_context.m_initialized = true;
}

void ErDeinit(void)
{
    ER_ASSERT(s_context.m_initialized);
    memset(&s_context, 0, sizeof(s_context));
}

void ErSendEx(ErEvent_t *a_event, ErSendExOptions_t a_options)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(a_event != NULL);
    ER_ASSERT(IsEventSendable(a_event));

    // Modules are not allowed to subscribe to event types that they send. If
    // that were allowed then one event handler could receive an event twice in
    // response to one send action: once to deliver the event as part of the
    // subscription and a second time to indicate the event is free.

    const BitRef_t module_bit_ref =
        GetBitRef((atomic_char *)a_event->m_sending_module->m_subscriptions,
                  a_event->m_type);

    const bool sending_module_subscribed =
        *module_bit_ref.m_byte & module_bit_ref.m_bit_mask;
    ER_ASSERT(!sending_module_subscribed);

    // When an event is sent its reference count is incremented by the number of
    // tasks that should receive the event plus one; each task that receives the
    // event will call `ErReturnToSender()` and the "plus one" covers posting
    // the event back to the sending module's task, which will call
    // `ErReturnToSender()` one more time.
    //
    // The reference count must be incremented BEFORE any events are sent to
    // queues to prevent the router from returning the event to the sending
    // module more than once.
    //
    // To explain consider sending events with the following pseudocode:
    //
    //     foreach(task) {
    //         if (IsSubscribed(task, event)) {
    //             IncrementReferenceCount(event);
    //             SendTo(task, event);
    //         }
    //     }
    //
    // When the loop reaches the first task with a subscription it increments
    // the count and posts the event to its queue. If that task has a higher
    // priority than the sending task the scheduler switches to that task
    // immediately and delivers the event. That task processes the event and
    // calls `ErReturnToSender()`, which decrements the counter, notices that it
    // is zero, and posts the event back to the senders task. Control then
    // returns to the loop where the same thing can happen again.
    //
    // This violates the guarantee that the event router returns exactly one
    // copy of an event back to the module that sends it.
    //
    // Iterating over the task list twice, once to increment the reference
    // counter and a second time to post events, is also incorrect. This is
    // because subscriptions may change between the first loop and the second
    // and that can make the amount added to the reference counter different
    // from the number of tasks the event is sent to.
    //
    // The correct solution is to iterate over the tasks once, simultaneously
    // counting the interested tasks and marking them for sending. After that we
    // increment the reference counter all at once and then send the event to
    // all marked tasks.
    //
    // If subscriptions change between marking tasks and sending events it isn't
    // a problem. If a task gets an event that it doesn't want it will be
    // ignored. If a task misses out on getting this event because it was too
    // late, too bad; it will get the next event of this type.

    // Count and mark tasks which should receive this event.
    uint32_t subscribed_task_mask = 0;
    ER_STATIC_ASSERT(
        (sizeof(subscribed_task_mask) * CHAR_BIT) >= TASK_SEND_LIMIT,
        "There must be enough bits in the mask to mark all the tasks");
    size_t subscribed_task_count = 0;
    for (size_t idx = 0; idx < s_context.m_options->m_num_tasks; ++idx)
    {
        const ErTask_t *task = &s_context.m_options->m_tasks[idx];
        const BitRef_t bit_ref =
            GetBitRef((atomic_char *)task->m_subscriptions, a_event->m_type);
        const bool task_is_subscribed =
            atomic_load(bit_ref.m_byte) & bit_ref.m_bit_mask;

        if (task_is_subscribed)
        {
            subscribed_task_count += 1;
            subscribed_task_mask |= 1 << idx;
        }
    }

    // Update the reference count to account for each event we plan to send to
    // subscribed tasks. The atomic increment returns the previous reference
    // count which tells us what else is happening in the system.

    const int old_reference_count =
        atomic_fetch_add(&a_event->m_reference_count, subscribed_task_count);

    // The reference count may NEVER go negative.
    ER_ASSERT(old_reference_count >= 0);

    const size_t sending_task_idx = a_event->m_sending_module->m_task_idx;
    const ErTask_t *sending_task =
        &s_context.m_options->m_tasks[sending_task_idx];

    // NOTE: This block is the trickiest logic in the module; any modifications
    // to it require careful consideration and heavy testing.
    if (old_reference_count == 0)
    {
        // If the reference count was zero before the increment the event is
        // IDLE; either it has never been sent, or it has been received by the
        // sender as many times as it has been sent. In either case, there is no
        // risk of a race condition.
        //
        // Add 1 to the reference count to account for sending the event back to
        // the sending task after delivering it to all subscribers.
        atomic_fetch_add(&a_event->m_reference_count, 1);

        // If there are no subscribers the sending task must still receive a
        // copy of the event. Send the event here and exit the function.
        if (subscribed_task_count == 0)
        {
            s_context.m_os_functions.SendEvent(sending_task->m_event_queue,
                                               a_event);
            return;
        }
    }
    else if (old_reference_count == 1)
    {
        // The event was already sent, make sure it can be re-sent.
        ER_ASSERT(EventResendingAllowed(&a_options, sending_task_idx));

        // If the old reference count is 1, then all subscribers from the
        // previous send received the event, the last subscriber's task has
        // called `ErReturnToSender()`, the atomic decrement in that function
        // has run, and that function has committed to sending the event back to
        // the sending module's task.
        //
        // This tells us two things which are crucial to understand:
        //
        // 1. There is now (or will be shortly) a copy of this event in the
        //    sending module's task's queue.
        //
        // 2. The 1 the previous send added to the reference count to account
        //    for sending the event back to the sending task has been consumed.

        if (subscribed_task_count == 0)
        {
            // If there are no subscribers then this function must send an event
            // back to the sending task. According to 1., that event already
            // exists so we can do nothing.
        }
        else if (subscribed_task_mask & (1 << sending_task_idx))
        {
            // There are subscribers and at least one of them is in the sending
            // module's task. Normally this requires sending an event to the
            // sending module's task and incrementing the reference count by 1
            // to account for the return trip.
            //
            // According to 1., there is already an event en route to the
            // sending module's task. We will use that event instead of sending
            // a new one by removing the sending task from the
            // `subscribed_task_mask` and decrementing the count.
            //
            // The astute reader may have noticed that we already incremented
            // the reference count by `subscribed_task_count` above and might
            // wonder whether that's a problem. The answer is no, there is no
            // problem. The (now) extra 1 in the increment above the 1 that we
            // would have added here to account for the return trip.
            //
            // This is more than an optimization; absent this, either
            // subscribers in the sending module's task will receive one more
            // event than subscribers in other tasks, or the event won't be
            // returned to the sender.
            //
            // This case is what imposes the requirement that clients who resend
            // an event must do so from the sending module's task.
            subscribed_task_mask &= ~(1 << sending_task_idx);
            subscribed_task_count -= 1;
        }
        else
        {
            // There are subscribers but none of them are in the sending
            // module's task. The only thing needed is increment the reference
            // count by 1 to account for the return trip.
            atomic_fetch_add(&a_event->m_reference_count, 1);
        }
    }
    else
    {
        // The event was already sent, make sure it can be re-sent.
        ER_ASSERT(EventResendingAllowed(&a_options, sending_task_idx));

        // The event is already in flight but it has not yet consumed the 1 in
        // the reference count dedicated to returning it to the sending module's
        // task. There is nothing else to do here.
    }

    // Deliver the event to the marked tasks. Since tasks are listed from
    // highest-priority to lowest they are delivered in priority order.
    for (size_t idx = 0; idx < s_context.m_options->m_num_tasks; ++idx)
    {
        if (subscribed_task_mask & (1 << idx))
        {
            s_context.m_os_functions.SendEvent(
                s_context.m_options->m_tasks[idx].m_event_queue, a_event);
        }
    }
}

void ErSend(ErEvent_t *a_event)
{
    const ErSendExOptions_t options = {.m_allow_resending = false};
    ErSendEx(a_event, options);
}

void ErCallHandlers(ErEvent_t *a_event)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(a_event != NULL);
    ER_ASSERT(IsEventTypeRoutable(a_event->m_type));

    /// This logic is needed to handle the case where the sending module and
    /// subscribing modules live in the same task. If the reference count is
    /// less than or equal to 1 this event is in the process of being returned
    /// and should not be delivered to subscribing modules.
    ///
    /// The sending module's handler is called by ErReturnToSender().
    if (atomic_load(&a_event->m_reference_count) <= 1)
    {
        goto done;
    }

    const size_t task_idx = GetIndexOfCurrentTask();
    const ErTask_t *task  = &s_context.m_options->m_tasks[task_idx];

    for (size_t module_idx = 0; module_idx < task->m_num_modules; ++module_idx)
    {
        ErModule_t *module = task->m_modules[module_idx];
        const BitRef_t module_bit_ref =
            GetBitRef((atomic_char *)module->m_subscriptions, a_event->m_type);
        const bool module_is_subscribed =
            *module_bit_ref.m_byte & module_bit_ref.m_bit_mask;

        // The subscription check occurs well after this event was sent to this
        // task with `ErSend()`. If a module unsubscribes to this
        // event type after the event was sent, but before it was delivered, it
        // will not receive it. This means unsubscription is instantaneous; once
        // a module unsubscribes from an event type it will not receive another
        // event of that type event if one was already on its way.

        if (module_is_subscribed)
        {
            // Deliver the event to the subscribed module.
            const ErEventHandlerRet_t ret = module->m_handler(a_event);

            if (ret == ER_EVENT_HANDLER_RET__KEPT)
            {
                // If a module keeps a reference to an event it is
                // responsible for calling `ErReturnToSender()`. We account
                // for this extra call by incrementing the reference count.
                atomic_fetch_add(&a_event->m_reference_count, 1);
            }

            // NOTE: This is a good place to put diagnostic information
            // about how event handlers respond to events.
        }
    }

done:
    ErReturnToSender(a_event);
}

void ErReturnToSender(ErEvent_t *a_event)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(a_event != NULL);
    ER_ASSERT(IsEventSendable(a_event));

    const int previous_reference_count =
        atomic_fetch_sub(&a_event->m_reference_count, 1);
    const int reference_count = previous_reference_count - 1;

    if (reference_count > 1)
    {
        // Do nothing. This event was sent to multiple tasks and this task (or
        // module) finished working with it before some others. Only the last
        // task (or module) to hold the reference to an event needs to return
        // the event to the sender.
    }
    else if (reference_count == 1)
    {
        // All the recipient tasks are done with the event. We must return the
        // event to its sender.

        const size_t sending_task_idx = a_event->m_sending_module->m_task_idx;

        // The sending task is different from the current task, so we need to
        // send it to that task's queue.
        if (sending_task_idx != GetIndexOfCurrentTask())
        {
            s_context.m_os_functions.SendEvent(
                s_context.m_options->m_tasks[sending_task_idx].m_event_queue,
                a_event);

            // The `return` below is necessary to prevent double-delivery of
            // events to the sending module. The problematic case, without this
            // `return`, is as follows:
            //
            // 1. This task is the *last* task with a subscribing module.
            // 2. This task has a lower priority than the sending module's task.
            // 3. The call to SendEvent() switches focus to the sending task.
            // 4. The sending task decrements the reference count from 1 to 0.
            // 5. The sending task delivers the event to the sending module.
            // 6. Control returns to this task.
            // 7. Code continues to reference count check below.
            // 8. The reference count equals zero.
            // 9. This task delivers the event to the sending module again.

            return;
        }
        else
        {
            // The module that we need to return this event to is in this task.
            //
            // Avoid an extra send by decrementing the reference count (as if we
            // sent it back) and then delivering the event to the sending
            // module's event handler below.
            ER_ASSERT(1 == atomic_fetch_sub(&a_event->m_reference_count, 1));
        }
    }

    // Deliver the event to the sending module's event handler.
    //
    // This block is intentionally disconnected from the if-else blocks above to
    // support the optimization mentioned a few lines up.
    if (atomic_load(&a_event->m_reference_count) == 0)
    {
        a_event->m_sending_module->m_handler(a_event);
    }
}

void ErSubscribe(ErModule_t *a_module, ErEventType_t a_event_type)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(IsModuleOwned(a_module));
    ER_ASSERT(IsEventTypeRoutable(a_event_type));

    // Set the subscription bit for this module.
    const BitRef_t module_bit_ref =
        GetBitRef((atomic_char *)a_module->m_subscriptions, a_event_type);
    atomic_fetch_or(module_bit_ref.m_byte, module_bit_ref.m_bit_mask);

    // Set the subscription bit for the task that owns this module.
    const ErTask_t *task = &s_context.m_options->m_tasks[a_module->m_task_idx];
    const BitRef_t task_bit_ref =
        GetBitRef((atomic_char *)task->m_subscriptions, a_event_type);
    atomic_fetch_or(task_bit_ref.m_byte, task_bit_ref.m_bit_mask);
}

void ErUnsubscribe(ErModule_t *a_module, ErEventType_t a_event_type)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(IsModuleOwned(a_module));
    ER_ASSERT(IsEventTypeRoutable(a_event_type));

    // Clear the subscription bit for this module.
    const BitRef_t bit_ref =
        GetBitRef((atomic_char *)a_module->m_subscriptions, a_event_type);
    // This module owns this memory so there is no need for atomic access.
    *bit_ref.m_byte &= ~bit_ref.m_bit_mask;

    // Clear the task's subscription bit if none of its modules are subscribed.
    const ErTask_t *task = &s_context.m_options->m_tasks[a_module->m_task_idx];
    bool any_subscriptions = false;
    for (size_t module_idx = 0; module_idx < task->m_num_modules; ++module_idx)
    {
        ErModule_t *module = task->m_modules[module_idx];
        const BitRef_t module_bit_ref =
            GetBitRef((atomic_char *)module->m_subscriptions, a_event_type);
        // All the module subscription bits accessed in this loop are owned by
        // modules which are owned by the same task. Since they run in the same
        // task they cannot subscribe or unsubscribe during this call to
        // `ErUnsubscribe()`; there is no need for atomic access.
        const bool module_is_subscribed =
            *module_bit_ref.m_byte & module_bit_ref.m_bit_mask;

        if (module_is_subscribed)
        {
            any_subscriptions = true;
            break;
        }
    }

    if (!any_subscriptions)
    {
        // Task bits CAN be accessed concurrently; atomic operations are
        // necessary.
        const BitRef_t task_bit_ref =
            GetBitRef((atomic_char *)task->m_subscriptions, a_event_type);
        atomic_fetch_and(task_bit_ref.m_byte, ~task_bit_ref.m_bit_mask);
    }
}

ErEvent_t *ErReceive(void)
{
    const ErTask_t *task =
        &s_context.m_options->m_tasks[GetIndexOfCurrentTask()];
    ErEvent_t *event = NULL;
    s_context.m_os_functions.ReceiveEvent(task->m_event_queue, &event);
    ER_ASSERT(event != NULL);
    return event;
}

ErEvent_t *ErTimedReceive(int64_t a_ms)
{
    const ErTask_t *task =
        &s_context.m_options->m_tasks[GetIndexOfCurrentTask()];
    ErEvent_t *event = NULL;
    s_context.m_os_functions.TimedReceiveEvent(task->m_event_queue, &event, a_ms);
    return event;
}

void ErSetOsFunctions(const ErOsFunctions_t *a_fns)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(a_fns != NULL);
    ER_ASSERT(a_fns->SendEvent != NULL);
    ER_ASSERT(a_fns->ReceiveEvent != NULL);
    ER_ASSERT(a_fns->TimedReceiveEvent != NULL);
    ER_ASSERT(a_fns->GetCurrentTaskHandle != NULL);

    s_context.m_os_functions = *a_fns;
}
