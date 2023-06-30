#include "mock_os.h"

ErOptions_t MockOs::m_event_router_options;
ErTaskHandle_t MockOs::m_running_task = 0;
std::unordered_map<ErQueueHandle_t, std::queue<ErEvent_t *>>
    MockOs::m_sent_events{};
constexpr ErOsFunctions_t MockOs::m_os_functions;
int64_t MockOs::m_now_ms;
