#include "mock_os.h"

ErOptions_t MockOs::m_event_router_options;
TaskHandle_t MockOs::m_running_task = nullptr;
std::unordered_map<QueueHandle_t, std::queue<ErEvent_t *>>
    MockOs::m_sent_events{};
constexpr ErOsFunctions_t MockOs::m_os_functions;
int64_t MockOs::m_now_ms;
