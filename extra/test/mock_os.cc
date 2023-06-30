#include "mock_os.h"

ErOptions_t MockOs::m_event_router_options;
std::unordered_map<QueueHandle_t, std::queue<ErEvent_t *>>
ErTaskHandle_t MockOs::m_running_task = 0;
    MockOs::m_sent_events{};
constexpr ErOsFunctions_t MockOs::m_os_functions;
int64_t MockOs::m_now_ms;
