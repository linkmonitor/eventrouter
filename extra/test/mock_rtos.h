#ifndef MOCK_RTOS_H
#define MOCK_RTOS_H

/// This structure is useful for simulating portions of FreeRTOS and setting up
/// the Event Router in unit tests.

#ifndef __cplusplus
#error "This module only supports C++"
#endif

#include <queue>
#include <unordered_map>

#include <string.h>

#include "eventrouter.h"
#include "eventrouter/internal/rtos_functions.h"

/// Implements the RTOS interactions used by the Event Router.
struct MockRtos
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

    //==========================================================================
    // The following three functions fill out a
    // `ErRtosFunctions_t` struct and either capture their
    // arguments or allow tests to specify their return values.
    // ==========================================================================

    static void SendEvent(QueueHandle_t a_queue, void *a_event)
    {
        m_sent_events[a_queue].push((ErEvent_t *)a_event);
    }

    static TaskHandle_t GetCurrentTaskHandle() { return m_running_task; }

    static ErOptions_t m_event_router_options;
    static TaskHandle_t m_running_task;
    static std::unordered_map<QueueHandle_t, std::queue<ErEvent_t *>>
        m_sent_events;
    static constexpr ErRtosFunctions_t m_rtos_functions = {
        .SendEvent            = SendEvent,
        .GetCurrentTaskHandle = GetCurrentTaskHandle,
    };
    static int64_t m_now_ms;
};

#endif /* MOCK_RTOS_H */
