#ifndef MOCK_OS_H
#define MOCK_OS_H

/// This structure is useful for simulating the portions of operating systems
/// used by the Event Router in unit tests

#ifndef __cplusplus
#error "This module only supports C++"
#endif

#include <queue>
#include <unordered_map>

#include <string.h>

#include "eventrouter.h"
#include "eventrouter/internal/defs.h"
#include "eventrouter/internal/os_functions.h"

/// Implements the OS interactions used by the Event Router.
struct MockOs
{
    static void Init(const ErOptions_t *a_options)
    {
        m_now_ms               = 0;
        m_event_router_options = *a_options;
        m_running_task         = nullptr;
        m_sent_events.clear();
    }

    static void SwitchTask(TaskHandle_t a_task) { m_running_task = a_task; }

    static ErEvent_t *ReceiveEvent()
    {
        QueueHandle_t running_task_queue = nullptr;
        for (size_t idx = 0; idx < m_event_router_options.m_num_tasks; ++idx)
        {
            const ErTask_t *task = &m_event_router_options.m_tasks[idx];
            if (task->m_task_handle == m_running_task)
            {
                running_task_queue = task->m_event_queue;
                break;
            }
        }
        assert(running_task_queue != nullptr);
        auto &queue = m_sent_events[running_task_queue];
        assert(queue.size() > 0);

        ErEvent_t *event = queue.front();
        m_sent_events[running_task_queue].pop();
        return event;
    }

    static bool AnyUnhandledEvents()
    {
        for (auto &entry : m_sent_events)
        {
            if (!entry.second.empty())
            {
                return true;
            }
        }
        return false;
    }

    static void ClearUnhandledEvents() { m_sent_events.clear(); }

    static int64_t GetTimeMs(void) { return m_now_ms; }
    static void AdvanceTimeMs(TickType_t a_delta_ms) { m_now_ms += a_delta_ms; }

    static bool IsInIsr(void) { return false; }

    static ErOptions_t m_event_router_options;
    static TaskHandle_t m_running_task;
    static std::unordered_map<QueueHandle_t, std::queue<ErEvent_t *>>
        m_sent_events;
    static int64_t m_now_ms;

    //==========================================================================
    // These functions populate a `ErOsFunctions_t` struct and either capture
    // arguments or allow tests to specify return values.
    // ==========================================================================

    static void SendEvent(QueueHandle_t a_queue, void *a_event)
    {
        m_sent_events[a_queue].push((ErEvent_t *)a_event);
    }

    static TaskHandle_t GetCurrentTaskHandle() { return m_running_task; }

    static void ReceiveEvent(QueueHandle_t a_queue, ErEvent_t **a_event)
    {
        assert(a_queue != nullptr);
        assert(a_event != nullptr);
        auto &queue = m_sent_events[a_queue];
        assert(queue.size() > 0);
        ErEvent_t *event = queue.front();
        queue.pop();
        *a_event = event;
    }

    static bool TimedReceiveEvent(QueueHandle_t a_queue, ErEvent_t **a_event,
                                  int64_t a_ms)
    {
        // We don't have good way to test blocking calls presently.
        ER_UNUSED(a_ms);
        ReceiveEvent(a_queue, a_event);
        return true;
    }

    static constexpr ErOsFunctions_t m_os_functions = {
        .SendEvent            = SendEvent,
        .ReceiveEvent         = ReceiveEvent,
        .TimedReceiveEvent    = TimedReceiveEvent,
        .GetCurrentTaskHandle = GetCurrentTaskHandle,
    };
};

#endif /* MOCK_OS_H */
