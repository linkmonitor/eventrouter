#include "mock_rtos.h"

ErOptions_t MockRtos::m_event_router_options;
TaskHandle_t MockRtos::m_running_task = nullptr;
std::unordered_map<QueueHandle_t, std::queue<ErEvent_t *>>
    MockRtos::m_sent_events{};
constexpr ErOsFunctions_t MockRtos::m_os_functions;
int64_t MockRtos::m_now_ms;
