#include "eventrouter.h"

#include <stdbool.h>
#include <string.h>

#include "bitref.h"
#include "checked_config.h"
#include "list.h"

static struct
{
    bool m_initialized;
    const ErOptions_t *m_options;
    struct
    {
        ErList_t m_deliver_now;   // Deliver this iteration of the main loop.
        ErList_t m_deliver_next;  // Deliver on the next iteration.
        ErList_t m_kept;          // Events which modules have kept.
    } m_events;
} s_context;

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

void ErInit(const ErOptions_t *a_options)
{
    ER_ASSERT(!s_context.m_initialized);
    // Baremetal event routers must have EXACTLY one task and at least one
    // module in that task. The only reason to keep the task abstraction at all
    // is to reuse function signatures and type definitions.
    ER_ASSERT(a_options != NULL);
    ER_ASSERT(a_options->m_num_tasks == 1);
    const ErTask_t *task = &a_options->m_tasks[0];
    ER_ASSERT(task->m_num_modules > 0);

    for (size_t idx = 0; idx < task->m_num_modules; ++idx)
    {
        ErModule_t *module   = task->m_modules[idx];
        ER_ASSERT(module->m_handler != NULL);
        module->m_task_idx   = 0;
        module->m_module_idx = idx;
        memset(&module->m_subscriptions, 0, sizeof(module->m_subscriptions));
    }

    s_context.m_options     = a_options;
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
    ER_ASSERT(!a_options.m_allow_resending);

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

    /// Re-sending  is not allowed in the baremetal implementation.
    ER_ASSERT(!ErEventIsInFlight(a_event));
    ER_ASSERT(a_event->m_next.m_next == NULL);

    /// Prepare to deliver the event on the next iteration of the main loop,
    /// even if only to the sending module.
    a_event->m_reference_count++;
    ErListAppend(&s_context.m_events.m_deliver_next, &a_event->m_next);
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

    const ErTask_t *task     = &s_context.m_options->m_tasks[0];
    const size_t num_modules = task->m_num_modules;
    for (size_t module_idx = 0; module_idx < num_modules; ++module_idx)
    {
        ErModule_t *module = task->m_modules[module_idx];
        const BitRef_t module_bit_ref =
            GetBitRef((atomic_char *)module->m_subscriptions, a_event->m_type);
        const bool module_is_subscribed =
            *module_bit_ref.m_byte & module_bit_ref.m_bit_mask;

        if (module_is_subscribed)
        {
            // Deliver the event to the subscribed module.
            const ErEventHandlerRet_t ret = module->m_handler(a_event);

            if (ret == ER_EVENT_HANDLER_RET__KEPT)
            {
                // If a module keeps a reference to an event it is
                // responsible for calling `ErReturnToSender()`. We account
                // for this extra call by incrementing the reference count.
                a_event->m_reference_count++;

                // This list exists for debugging purposes. If an event is never
                // returned to its sender and is in this list then a module kept
                // an event and never called ErReturnToSender().
                ErListAppend(&s_context.m_events.m_kept, &a_event->m_next);
            }

            // NOTE: This is a good place to put diagnostic information
            // about how event handlers respond to events.
        }
    }

    /// This event no longer needs delivery.
    ErListRemove(&s_context.m_events.m_deliver_now, &a_event->m_next);

done:
    ErReturnToSender(a_event);
}

void ErReturnToSender(ErEvent_t *a_event)
{
    ER_ASSERT(s_context.m_initialized);
    ER_ASSERT(a_event != NULL);
    ER_ASSERT(IsEventSendable(a_event));

    a_event->m_reference_count--;

    if (a_event->m_reference_count > 0)
    {
        // Do nothing. Some modules have KEPT the event and must explicitly
        // call `ErReturnToSender()` before we can return it.
    }
    else if (a_event->m_reference_count == 0)
    {
        // Remove the event from the KEPT list (no-op if the event wasn't kept).
        ErListRemove(&s_context.m_events.m_kept, &a_event->m_next);

        // All subscribed modules have received the event; return to its sender.
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
}

void ErNewLoop(void)
{
    /// Keep events which may not have been delivered during the previous loop
    /// at the head of the "deliver now" list and add events which were
    /// scheduled for delivery during the previous loop.
    ErListAppend(&s_context.m_events.m_deliver_now,
                 s_context.m_events.m_deliver_next.m_next);
    /// Clear the "deliver next" list so it can be filled during this loop and
    /// delivered during the next loop.
    s_context.m_events.m_deliver_next.m_next = NULL;
}

ErEvent_t *ErGetEventToDeliver(void)
{
    ErEvent_t *ret = NULL;

    ErList_t *node = &s_context.m_events.m_deliver_now;
    if (node->m_next != NULL)
    {
        ret          = container_of(node->m_next, ErEvent_t, m_next);
        node->m_next = node->m_next->m_next;
    }

    return ret;
}
