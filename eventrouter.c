#include "eventrouter/eventrouter.h"
#include "eventrouter/eventrouter_internal.h"

#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "eventrouter/defs.h"

/// The maximum number of tasks the Event Router can support without changing
/// the dispatch strategy in `ErSend()`.
#define TASK_SEND_LIMIT (32)

/// A two-dimensional array of bits where each of the `m_num_entries` is an
/// array of `m_bits_per_entry`.
typedef struct
{
    size_t m_num_entries;
    size_t m_bits_per_entry;
    atomic_char m_bytes[];  // Flexible array member;
} BitArray2D_t;

_Static_assert(ATOMIC_CHAR_LOCK_FREE == 2,
               "Accessing an atomic char must never involve a mutex");

/// A reference to a specific bit within a `BitArray2D_t`.
typedef struct
{
    atomic_char *m_byte;
    uint8_t m_bit_mask;
} BitRef_t;

/// File-specific state. This includes a reference to the options which are
/// passed in at initialization and references to the memory which is allocated.
static struct
{
    bool m_initialized;
    const ErOptions_t *m_options;
    BitArray2D_t *m_task_subscriptions;
    BitArray2D_t *m_module_subscriptions;
    ErRtosFunctions_t m_rtos_functions;
} s_context;

static bool IsInIsr(void)
{
    ER_ASSERT(s_context.m_initialized);
    return s_context.m_options->m_system_funcs.m_IsInIsr();
}

static void DefaultSendEvent(QueueHandle_t a_queue, void *a_event)
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

static TaskHandle_t DefaultGetCurrentTaskHandle(void)
{
    return xTaskGetCurrentTaskHandle();
}

static void DefaultGetTaskInfo(TaskHandle_t a_task, TaskStatus_t *a_status)
{
    ER_UNUSED(a_task);
    *a_status = (TaskStatus_t){0};

#if (configUSE_TRACE_FACILITY == 1)
    vTaskGetTaskInfo(a_task, a_status, pdFALSE, eRunning);
#endif
}

static BitArray2D_t *AllocateBitArray(size_t a_num_entries,
                                      size_t a_bits_per_entry)
{
    // Use the custom malloc if given and `malloc()` if not.
    typeof(malloc) *const er_malloc =
        s_context.m_options->m_system_funcs.m_Malloc
            ? s_context.m_options->m_system_funcs.m_Malloc
            : malloc;

    /// Round the bits per entry up to the nearest byte to so
    /// different entries never access bits in the same byte. This
    /// makes it safe for modules to access their subscription bits
    /// without atomic operations.
    const size_t bytes_per_entry =
        (a_bits_per_entry + (CHAR_BIT - 1)) / CHAR_BIT;
    const size_t num_data_bytes = a_num_entries * bytes_per_entry;

    BitArray2D_t *ret = er_malloc(sizeof(BitArray2D_t) + num_data_bytes);
    ER_ASSERT(ret != NULL);
    ret->m_num_entries    = a_num_entries;
    ret->m_bits_per_entry = bytes_per_entry * CHAR_BIT;
    memset(ret->m_bytes, 0, num_data_bytes);
    return ret;
}

/// Return a reference to the byte containing the bit specified along with a
/// mask identifying the bit in that byte.
///
/// This is indirect, but it lets clients to choose how they want to read or
/// modify the byte/bit in question. Some clients will use atomic accessors,
/// others won't.
static BitRef_t GetBitRef(BitArray2D_t *a_array, size_t a_entry, size_t a_bit)
{
    ER_ASSERT(a_entry <= a_array->m_num_entries);

    BitRef_t ref = {0};

    const size_t entry_offset = a_array->m_bits_per_entry * a_entry;
    const size_t bit_index    = entry_offset + a_bit;
    const size_t byte_index   = bit_index / CHAR_BIT;
    const size_t bit_mask     = (1 << (bit_index % CHAR_BIT));

    ref.m_byte     = &a_array->m_bytes[byte_index];
    ref.m_bit_mask = bit_mask;

    return ref;
}

/// Allocates the memory needed to track subscriptions based on the information
/// in `a_options`. Free this memory with `FreeContextMemory()`.
static void AllocateContextMemory(const ErOptions_t *a_options)
{
    ER_ASSERT(!s_context.m_initialized);

    s_context.m_options = a_options;

    // Each task/module needs space to subscribe to each event type.
    s_context.m_task_subscriptions =
        AllocateBitArray(a_options->m_num_tasks, a_options->m_num_event_types);
    s_context.m_module_subscriptions = AllocateBitArray(
        a_options->m_num_event_handlers, a_options->m_num_event_types);

    s_context.m_rtos_functions = (ErRtosFunctions_t){
        .SendEvent            = DefaultSendEvent,
        .GetCurrentTaskHandle = DefaultGetCurrentTaskHandle,
        .GetTaskInfo          = DefaultGetTaskInfo,
    };

    s_context.m_initialized = true;
}

static void FreeContextMemory(void)
{
    ER_ASSERT(s_context.m_initialized);

    // Use the custom free if given and `free()` if not.
    typeof(free) *const er_free =
        s_context.m_options->m_system_funcs.m_Free
            ? s_context.m_options->m_system_funcs.m_Free
            : free;

    er_free(s_context.m_task_subscriptions);
    er_free(s_context.m_module_subscriptions);
    memset(&s_context, 0, sizeof(s_context));
}

/// The options for the event router have several requirements. This function
/// contains the logic to verify that `a_options` meets those requirements and
/// asserts if they are not met.
static void AssertOptionsAreValid(const ErOptions_t *a_options)
{
    ER_ASSERT(a_options != NULL);
    ER_ASSERT(a_options->m_tasks != NULL);
    ER_ASSERT(a_options->m_event_handlers != NULL);
    ER_ASSERT(a_options->m_system_funcs.m_IsInIsr != NULL);

    // There must be at least one event type to route.
    ER_ASSERT(a_options->m_num_event_types > 0);

    // All event handlers must be non-NULL.
    for (size_t idx = 0; idx < a_options->m_num_event_handlers; ++idx)
    {
        ER_ASSERT(a_options->m_event_handlers[idx] != NULL);
    }

    // See the comment in `ErEventRouterSend()` for more info.
    ER_ASSERT(a_options->m_num_tasks <= TASK_SEND_LIMIT);

    // All task descriptions must be valid. This means they:
    // 1. Have a queue to deliver events to;
    // 2. Have a valid task handle; and
    // 3. Claim ownership over a valid range of modules.
    ErModuleId_t previous_tasks_last_id;
    for (size_t idx = 0; idx < a_options->m_num_tasks; ++idx)
    {
        const ErTaskDesc_t *task = &a_options->m_tasks[idx];
        ER_ASSERT(task->m_event_queue != NULL);
        ER_ASSERT(task->m_task_handle != NULL);

        // Module IDs are actually addresses of entries in the list of event
        // handlers. They are defined this way to guarantee uniqueness and to
        // make them hard for modules to guess.
        const ErModuleId_t first_valid_id =
            (ErModuleId_t)&a_options->m_event_handlers[0];
        const ErModuleId_t last_valid_id =
            (ErModuleId_t)&a_options
                ->m_event_handlers[a_options->m_num_event_handlers - 1];

        // Tasks must claim ownership of a range of modules from the list. Each
        // task must own at least one module.
        const ErModuleId_t first_id = task->m_first_module;
        const ErModuleId_t last_id  = task->m_last_module;
        ER_ASSERT(first_id <= last_id);  // Equal when one module is owned.
        ER_ASSERT((first_id >= first_valid_id) && (first_id <= last_valid_id));
        ER_ASSERT((last_id >= first_valid_id) && (last_id <= last_valid_id));

        // The set of tasks must own all the modules in the list without
        // overlap. The list of modules is "task-ordered" meaning all the
        // modules for one task are grouped together, and are followed by the
        // modules for the next task in the list.
        //
        // This logic checks that the first task starts with the first module,
        // the last task ends with the last module, and that all tasks but the
        // first start where the task before them left off.
        if (idx == 0)
        {
            ER_ASSERT(first_id == first_valid_id);
        }
        else
        {
            if (idx == (a_options->m_num_tasks - 1))
            {
                ER_ASSERT(last_id == last_valid_id);
            }

            // Calculate task's expected first ID in the following steps:
            // 1. Convert the previous task's last ID to a handler.
            // 2. Find the address of the handler after that.
            // 3. Convert that handler back into an ID.
            const ErEventHandler_t *last_handler =
                (ErEventHandler_t *)previous_tasks_last_id;
            const ErEventHandler_t *expected_first_handler =
                (ErEventHandler_t *)last_handler + 1;
            const ErModuleId_t expected_first_id =
                (ErModuleId_t)expected_first_handler;

            ER_ASSERT(first_id == expected_first_id);
        }

        previous_tasks_last_id = last_id;
    }
}

/// Returns the last module ID from the list set at initialization. This
/// function must be called after initialization completes.
static ErModuleId_t LastModuleId(void)
{
    return (ErModuleId_t)&s_context.m_options
        ->m_event_handlers[s_context.m_options->m_num_event_handlers - 1];
}

/// Returns the first module ID from the list set at initialization. This
/// function must be called after initialization completes.
static ErModuleId_t FirstModuleId(void)
{
    return (ErModuleId_t)&s_context.m_options->m_event_handlers[0];
}

/// Returns true if this ID is in the list of modules set at initialization.
/// This function must be called after initialization completes.
static bool IsSenderIdKnown(ErModuleId_t a_id)
{
    return (a_id >= FirstModuleId()) && (a_id <= LastModuleId());
}

/// Returns the ID for module after the current one. Do not call on the last ID.
static ErModuleId_t NextModuleId(ErModuleId_t a_id)
{
    ER_ASSERT(a_id < LastModuleId());

    const ErEventHandler_t *handler      = (ErEventHandler_t *)a_id;
    const ErEventHandler_t *next_handler = (ErEventHandler_t *)handler + 1;
    const ErModuleId_t next_id           = (ErModuleId_t)next_handler;
    return next_id;
}

/// Returns true if this type is in the range set at initialization. This
/// function must be called after initialization completes.
static bool IsEventTypeRoutable(ErEventType_t a_type)
{
    const ErOptions_t *options = s_context.m_options;

    const size_t num_types         = options->m_num_event_types;
    const ErEventType_t first_type = options->m_first_event_type;
    const ErEventType_t last_type  = first_type + (num_types - 1);

    return (a_type >= first_type) && (a_type <= last_type);
}

/// Returns true if this event can be delivered to subscribers and returned.
/// This function must be called after initialization completes.
static bool IsEventSendable(ErEvent_t *a_event)
{
    return IsEventTypeRoutable(a_event->m_type) &&
           IsSenderIdKnown(a_event->m_sending_module);
}

/// Returns the index from the `s_context.m_options->m_event_handlers` that this
/// ID corresponds to. It is an error to call this function with an invalid ID
/// or before initialization completes.
static size_t GetModuleIndexFromId(ErModuleId_t a_id)
{
    ErEventHandler_t *handler_address = (ErEventHandler_t *)a_id;
    const ErEventHandler_t *handlers_start =
        s_context.m_options->m_event_handlers;
    return handler_address - handlers_start;  // Pointer math.
}

/// Returns the index of the task in `s_context.m_options->m_tasks` that owns
/// the module. Module ownership is checked while validating options and must
/// exist. This function must be called after initialization completes.
static size_t GetIndexOfTaskThatOwnsModule(ErModuleId_t a_id)
{
    int task_index = -1;
    for (size_t idx = 0; idx < s_context.m_options->m_num_tasks; ++idx)
    {
        const ErModuleId_t first_id =
            s_context.m_options->m_tasks[idx].m_first_module;
        const ErModuleId_t last_id =
            s_context.m_options->m_tasks[idx].m_last_module;

        if ((a_id >= first_id) && (a_id <= last_id))
        {
            task_index = idx;
            break;
        }
    }
    ER_ASSERT(task_index >= 0);  // All modules MUST be owned by a task.

    return task_index;
}

/// Returns the index of the task in `s_context.m_options->m_tasks` that
/// corresponds with the currently running task.
static size_t GetIndexOfCurrentTask(void)
{
    const ErOptions_t *options = s_context.m_options;
    const TaskHandle_t current_task =
        s_context.m_rtos_functions.GetCurrentTaskHandle();

    int task_index = -1;
    for (size_t idx = 0; idx < options->m_num_tasks; ++idx)
    {
        if (current_task == options->m_tasks[idx].m_task_handle)
        {
            task_index = idx;
            break;
        }
    }

    if (task_index == -1)
    {
        TaskStatus_t status;
        s_context.m_rtos_functions.GetTaskInfo(current_task, &status);
        if (s_context.m_options->m_system_funcs.m_LogError)
        {
            s_context.m_options->m_system_funcs.m_LogError(
                "Task %s:%p is not registered with the event router",
                current_task, status.pcTaskName);
        }
        ER_ASSERT(0);
    }

    return task_index;
}

/// Events may only be re-sent if re-sending is explicitly allowed and the
/// sender is either in an interrupt or the sending module's task.
static bool EventResendingAllowed(const ErSendExOptions_t *a_options,
                                  size_t a_sending_task_index)
{
    return a_options->m_allow_resending &&
           (IsInIsr() || (a_sending_task_index == GetIndexOfCurrentTask()));
}

void ErInit(const ErOptions_t *a_options)
{
    ER_ASSERT(!s_context.m_initialized);
    AssertOptionsAreValid(a_options);
    AllocateContextMemory(a_options);
}

void ErDeinit(void)
{
    ER_ASSERT(s_context.m_initialized);
    FreeContextMemory();
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

    const BitRef_t module_bit_ref = GetBitRef(
        s_context.m_module_subscriptions,
        GetModuleIndexFromId(a_event->m_sending_module), a_event->m_type);
    const bool sending_module_subscribed =
        *module_bit_ref.m_byte & module_bit_ref.m_bit_mask;
    ER_ASSERT(!sending_module_subscribed);

    // When an event is sent its reference count is incremented by the number of
    // tasks that should receive the event plus one; each task that receives the
    // event will call `ErEventRouterReturnToSender()` and the "plus one" covers
    // posting the event back to the sending module's task, which will call
    // `ErEventRouterReturnToSender()` one more time.
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
    // calls `ErEventRouterReturnToSender()`, which decrements the counter,
    // notices that it is zero, and posts the event back to the senders task.
    // Control then returns to the loop where the same thing can happen again.
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
    // The correct solution is to iterate over the tasks once time and to
    // simultaneously count the interested tasks and mark them for sending.
    // After that we increment the reference counter all at once. After that we
    // send the event to all marked tasks.
    //
    // If subscriptions change between marking tasks and sending events it isn't
    // a problem. If a task gets an event that it doesn't want it will be
    // ignored. If a task misses out on getting this event because it was too
    // late, too bad; it will get the next event of this type.

    // Count and mark tasks which should receive this event.
    uint32_t subscribed_task_mask = 0;
    _Static_assert(
        (sizeof(subscribed_task_mask) * CHAR_BIT) >= TASK_SEND_LIMIT,
        "There must be enough bits in the mask to mark all the tasks");
    size_t subscribed_task_count = 0;
    for (size_t idx = 0; idx < s_context.m_options->m_num_tasks; ++idx)
    {
        const BitRef_t bit_ref =
            GetBitRef(s_context.m_task_subscriptions, idx, a_event->m_type);
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
            const size_t sending_task_index =
                GetIndexOfTaskThatOwnsModule(a_event->m_sending_module);
            s_context.m_rtos_functions.SendEvent(
                s_context.m_options->m_tasks[sending_task_index].m_event_queue,
                a_event);
            return;
        }
    }
    else if (old_reference_count == 1)
    {
        // The event was already sent, make sure it can be re-sent.
        const size_t sending_task_index =
            GetIndexOfTaskThatOwnsModule(a_event->m_sending_module);
        ER_ASSERT(EventResendingAllowed(&a_options, sending_task_index));

        // If the old reference count is 1, then all subscribers from the
        // previous send received the event, the last subscriber's task has
        // called `ErEventRouterReturnToSender()`, the atomic decrement in that
        // function has run, and that function has committed to sending the
        // event back to the sending module's task.
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
        else if (subscribed_task_mask & (1 << sending_task_index))
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
            subscribed_task_mask &= ~(1 << sending_task_index);
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
        const size_t sending_task_index =
            GetIndexOfTaskThatOwnsModule(a_event->m_sending_module);
        ER_ASSERT(EventResendingAllowed(&a_options, sending_task_index));

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
            s_context.m_rtos_functions.SendEvent(
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

    const size_t task_index      = GetIndexOfCurrentTask();
    const ErOptions_t *options   = s_context.m_options;
    const ErTaskDesc_t task_desc = options->m_tasks[task_index];

    ErModuleId_t id = task_desc.m_first_module;
    while (1)  // All tasks own at least one module.
    {
        // Deliver the event to the module if it is subscribed.
        const size_t module_index     = GetModuleIndexFromId(id);
        const BitRef_t module_bit_ref = GetBitRef(
            s_context.m_module_subscriptions, module_index, a_event->m_type);
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
            const ErEventHandlerRet_t ret =
                options->m_event_handlers[module_index](a_event);

            if (ret == ER_EVENT_HANDLER_RET__KEPT)
            {
                // If a module keeps a reference to an event it is responsible
                // for calling `ErReturnToSender()`. We account for
                // this extra call by incrementing the reference count.
                atomic_fetch_add(&a_event->m_reference_count, 1);
            }

            // NOTE: This is a good place to put diagnostic information about
            // how event handlers respond to events.
        }

        if (id == task_desc.m_last_module)
        {
            break;
        }

        id = NextModuleId(id);
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

        const size_t sending_task_index =
            GetIndexOfTaskThatOwnsModule(a_event->m_sending_module);

        // The sending task is different from the current task, so we need to
        // send it to that task's queue.
        if (sending_task_index != GetIndexOfCurrentTask())
        {
            s_context.m_rtos_functions.SendEvent(
                s_context.m_options->m_tasks[sending_task_index].m_event_queue,
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
        const size_t module_index =
            GetModuleIndexFromId(a_event->m_sending_module);
        s_context.m_options->m_event_handlers[module_index](a_event);
    }
}

void ErSubscribe(ErModuleId_t a_id, ErEventType_t a_event_type)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(IsSenderIdKnown(a_id));
    ER_ASSERT(IsEventTypeRoutable(a_event_type));

    // Set the subscription bit for this module.
    const BitRef_t module_bit_ref =
        GetBitRef(s_context.m_module_subscriptions, GetModuleIndexFromId(a_id),
                  a_event_type);
    atomic_fetch_or(module_bit_ref.m_byte, module_bit_ref.m_bit_mask);

    // Set the subscription bit for the task that owns this module.
    const BitRef_t task_bit_ref =
        GetBitRef(s_context.m_task_subscriptions,
                  GetIndexOfTaskThatOwnsModule(a_id), a_event_type);
    atomic_fetch_or(task_bit_ref.m_byte, task_bit_ref.m_bit_mask);
}

void ErUnsubscribe(ErModuleId_t a_id, ErEventType_t a_event_type)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(IsSenderIdKnown(a_id));
    ER_ASSERT(IsEventTypeRoutable(a_event_type));

    const size_t task_index      = GetIndexOfTaskThatOwnsModule(a_id);
    const ErTaskDesc_t task_desc = s_context.m_options->m_tasks[task_index];

    // Clear the subscription bit for this module.
    const BitRef_t bit_ref =
        GetBitRef(s_context.m_module_subscriptions, GetModuleIndexFromId(a_id),
                  a_event_type);
    // This module owns this memory so there is no need for atomic access.
    *bit_ref.m_byte &= ~bit_ref.m_bit_mask;

    // Clear the task's subscription bit if none of its modules are subscribed.
    bool any_subscriptions = false;
    ErModuleId_t id        = task_desc.m_first_module;
    while (1)  // All tasks own at least one module.
    {
        const BitRef_t module_bit_ref =
            GetBitRef(s_context.m_module_subscriptions,
                      GetModuleIndexFromId(id), a_event_type);

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

        if (id == task_desc.m_last_module)
        {
            break;
        }

        id = NextModuleId(id);
    }

    if (!any_subscriptions)
    {
        // Task bits CAN be accessed concurrently; atomic operations are
        // necessary.
        const BitRef_t task_bit_ref =
            GetBitRef(s_context.m_task_subscriptions, task_index, a_event_type);
        atomic_fetch_and(task_bit_ref.m_byte, ~task_bit_ref.m_bit_mask);
    }
}

void ErSetRtosFunctions(const ErRtosFunctions_t *a_fns)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(a_fns != NULL);
    ER_ASSERT(a_fns->SendEvent != NULL);
    ER_ASSERT(a_fns->GetCurrentTaskHandle != NULL);
    ER_ASSERT(a_fns->GetTaskInfo != NULL);

    s_context.m_rtos_functions = *a_fns;
}
