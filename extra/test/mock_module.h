#ifndef MOCK_MODULE_H
#define MOCK_MODULE_H

#ifndef __cplusplus
#error "This module only supports C++"
#endif

#include "eventrouter.h"

/// This struct lets tests create and reference mock modules using integers.
/// Each integer results in a different instantiation of the module type.
///
/// Clients can instantiate and reference a mock modules as follows:
///
///    TEST(Example, UsingModules)
///    {
///        ErEvent_t event_1;
///        ErEvent_t event_2;
///
///        MockModule<0>::EventHandler(&event_1);
///        MockModule<1>::EventHandler(&event_2);
///
///        EXPECT_EQ(MockModule<0>::m_last_event_handled, &event_1);
///        EXPECT_EQ(MockModule<1>::m_last_event_handled, &event_2);
///    }
///
/// Note that the values in `FooModule` persists between tests, because the
/// instance of the template struct is globally defined. Test writers must take
/// care to set member variables in the template structs before checking them.
template <int N>
struct MockModule
{
    static void Reset()
    {
        m_last_event_handled = nullptr;
        m_event_handler_ret  = ER_EVENT_HANDLER_RET__UNEXPECTED;

        memset(&m_module, 0, sizeof(m_module));
        m_module.m_handler = EventHandler;
    }

    static ErEventHandlerRet_t EventHandler(ErEvent_t *a_event)
    {
        m_last_event_handled = a_event;
        return m_event_handler_ret;
    }

    static ErEvent_t *m_last_event_handled;
    static ErModule_t m_module;
    static ErEventHandlerRet_t m_event_handler_ret;
};

/// These definitions can be made `inline` in C++17.
template <int N>
ErEvent_t *MockModule<N>::m_last_event_handled = nullptr;

template <int N>
ErModule_t MockModule<N>::m_module =
    ER_CREATE_MODULE(MockModule<N>::EventHandler);

template <int N>
ErEventHandlerRet_t MockModule<N>::m_event_handler_ret =
    ER_EVENT_HANDLER_RET__UNEXPECTED;

#endif /* MOCK_MODULE_H */
