#include "eventrouter.h"

#include "eventrouter/internal/event_type.h"
#include "gtest/gtest.h"
#include "mock_module.h"

#ifdef ER_CONFIG_OS
#include "mock_os.h"
#endif

namespace
{
/// Builds an instance of `ErOptions_t` that `ErInit()` should accept without
/// complaint. The value returned by this function can be modified to inject
/// errors and test initialization.
struct MockOptions
{
   public:
    struct Module
    {
        static constexpr int A = 0;
        static constexpr int B = 1;
        static constexpr int C = 2;

        static constexpr int Invalid = 6;
    };

    MockOptions()
    {
        MockModule<Module::A>::Reset();
        MockModule<Module::B>::Reset();
        MockModule<Module::C>::Reset();
    }

    static bool m_is_in_isr;
    static bool IsInIsr(void) { return m_is_in_isr; }

    static constexpr int kNumModules   = 3;
    ErModule_t *m_modules[kNumModules] = {
        &MockModule<Module::A>::m_module,
        &MockModule<Module::B>::m_module,
        &MockModule<Module::C>::m_module,
    };

    /// These options only define one task because the tests in this file are
    /// common to all implementations and the baremetal implementation only
    /// supports single-task systems.

    ErTask_t m_task{
#ifdef ER_CONFIG_OS
        .m_task_handle = (TaskHandle_t)1,
        .m_event_queue = (QueueHandle_t)1,
#endif
        .m_modules     = m_modules,
        .m_num_modules = kNumModules,
    };

    ErOptions_t m_options{
        .m_tasks     = &m_task,
        .m_num_tasks = 1,
#ifdef ER_CONFIG_OS
        .m_IsInIsr   = IsInIsr,
        .m_GetTimeMs = MockOs::GetTimeMs,
#endif
    };
};

bool MockOptions::m_is_in_isr = false;

constexpr ErEvent_t kInvalidEvents[] = {
    // Invalid event type.
    {.m_type           = ER_EVENT_TYPE__INVALID,
     .m_sending_module = &MockModule<MockOptions::Module::A>::m_module},
    // Sending module is not tracked by the eventrouter.
    {.m_type           = ER_EVENT_TYPE__1,
     .m_sending_module = &MockModule<MockOptions::Module::Invalid>::m_module},
    // Sending module is NULL.
    {.m_type = ER_EVENT_TYPE__1, .m_sending_module = nullptr},
};

void CopyEvent(ErEvent_t &a_dst, const ErEvent_t &a_src)
{
    memcpy(&a_dst, &a_src, sizeof(a_dst));
}

void ResetEvent(ErEvent_t &a_event)
{
    constexpr ErEvent_t kValidEvent = {
        .m_type           = ER_EVENT_TYPE__1,
        .m_sending_module = &MockModule<MockOptions::Module::A>::m_module,
    };
    CopyEvent(a_event, kValidEvent);
}

}  // namespace

namespace testing
{

//==============================================================================
// Tests for `ErInit()/ErDeinit()`
// ==============================================================================

TEST(ErInit, DiesOnNullArguments)
{
    EXPECT_DEATH(ErInit(nullptr), ".*");
}

TEST(ErInit, AcceptsValidArguments)
{
    MockOptions options{};
    ErInit(&options.m_options);
    ErDeinit();
}

TEST(ErInit, DieOnDoubleInit)
{
    MockOptions options{};
    ErInit(&options.m_options);
    EXPECT_DEATH(ErInit(&options.m_options), ".*");
    ErDeinit();
}

TEST(ErInit, DiesOnNullEventHandler)
{
    MockOptions options{};
    options.m_options.m_tasks[0].m_modules[0]->m_handler = nullptr;
    EXPECT_DEATH(ErInit(&options.m_options), ".*");
}

TEST(ErDeinit, DiesIfCalledBeforeInit)
{
    EXPECT_DEATH(ErDeinit(), ".*");
}

TEST(ErDeinit, DiesIfDeinitTwice)
{
    MockOptions options{};
    ErInit(&options.m_options);
    ErDeinit();
    EXPECT_DEATH(ErDeinit(), ".*");
}

TEST(ErSend, DiesIfCalledBeforeInit)
{
    ErEvent_t event;
    ResetEvent(event);
    EXPECT_DEATH(ErSend(&event), ".*");
}

TEST(ErSendEx, DiesIfCalledBeforeInit)
{
    ErEvent_t event;
    ResetEvent(event);
    EXPECT_DEATH(ErSendEx(&event, {}), ".*");
}

//==============================================================================
// Tests which assume initialization has completed successfully.
//==============================================================================

class EventRouterTest : public Test
{
   protected:
    EventRouterTest()
    {
        ErInit(&m_options.m_options);
#ifdef ER_CONFIG_OS
        ErSetOsFunctions(&MockOs::m_os_functions);
        MockOs::Init(&m_options.m_options);
        MockOs::SwitchTask(m_options.m_options.m_tasks[0].m_task_handle);
#endif
    }
    ~EventRouterTest()
    {
        // Tests must not end with undelivered events. The assumption is that
        // this represents a side-effect that test writers should handle
        // explicitly. If necessary, tests can call `MaybeDeliverEvent()` until
        // it returns `nullptr` if they want to empty the "queue".
        assert(!MaybeDeliverEvent());
        ErDeinit();
    }

    // Returns true when an event was delivered and false otherwise.
    bool MaybeDeliverEvent()
    {
        ErEvent_t *event = nullptr;
#ifdef ER_CONFIG_OS
        if (MockOs::AnyUnhandledEvents()) event = MockOs::ReceiveEvent();
#endif

#ifdef ER_BAREMETAL
        event = ErGetEventToDeliver();
#endif
        if (event) ErCallHandlers(event);
        return event;
    }

    // Must be called between the call to `ErSend()` and the corresponding all
    // to `MaybeDeliverEvent()`.
    void PrepareToDeliverEvents()
    {
#ifdef ER_BAREMETAL
        ErNewLoop();
#endif
    }

    MockOptions m_options;
};

TEST_F(EventRouterTest, SendDiesOnInvalidArguments)
{
    EXPECT_DEATH(ErSend(nullptr), ".*");

    for (const auto &invalid_event : kInvalidEvents)
    {
        ErEvent_t event;
        CopyEvent(event, invalid_event);
        EXPECT_DEATH(ErSend(&event), ".*");
    }
}

TEST_F(EventRouterTest, SendExDiesOnInvalidArguments)
{
    EXPECT_DEATH(ErSend(nullptr), ".*");

    for (const auto &invalid_event : kInvalidEvents)
    {
        ErEvent_t event;
        CopyEvent(event, invalid_event);
        EXPECT_DEATH(ErSendEx(&event, {}), ".*");
    }
}

TEST_F(EventRouterTest, CallHandlersDiesOnInvalidArguments)
{
    EXPECT_DEATH(ErSend(nullptr), ".*");

    for (const auto &invalid_event : kInvalidEvents)
    {
        ErEvent_t event;
        CopyEvent(event, invalid_event);
        EXPECT_DEATH(ErCallHandlers(&event), ".*");
    }
}

TEST_F(EventRouterTest, ReturnToSenderDiesOnInvalidArguments)
{
    EXPECT_DEATH(ErReturnToSender(nullptr), ".*");

    for (const auto &invalid_event : kInvalidEvents)
    {
        ErEvent_t event;
        CopyEvent(event, invalid_event);
        EXPECT_DEATH(ErReturnToSender(&event), ".*");
    }
}

TEST_F(EventRouterTest, SubscribeDiesOnInvalidArguments)
{
    EXPECT_DEATH(ErSubscribe(nullptr, ER_EVENT_TYPE__1), ".*");
    EXPECT_DEATH(
        ErSubscribe(&MockModule<MockOptions::Module::Invalid>::m_module,
                    ER_EVENT_TYPE__1),
        ".*");
    EXPECT_DEATH(ErSubscribe(&MockModule<MockOptions::Module::A>::m_module,
                             ER_EVENT_TYPE__INVALID),
                 ".*");
}

TEST_F(EventRouterTest, UnsubscribeDiesOnInvalidArguments)
{
    EXPECT_DEATH(ErUnsubscribe(nullptr, ER_EVENT_TYPE__1), ".*");
    EXPECT_DEATH(
        ErUnsubscribe(&MockModule<MockOptions::Module::Invalid>::m_module,
                      ER_EVENT_TYPE__1),
        ".*");
    EXPECT_DEATH(ErUnsubscribe(&MockModule<MockOptions::Module::A>::m_module,
                               ER_EVENT_TYPE__INVALID),
                 ".*");
}

TEST_F(EventRouterTest, RepeatedlyCallingPrepareToDeliverEventsIsSafe)
{
    // This tests the test-fixture; it should be safe to prepare to deliver
    // events multiple times without actually trying to delivering anything.

    PrepareToDeliverEvents();
    PrepareToDeliverEvents();
    PrepareToDeliverEvents();
    PrepareToDeliverEvents();
}

TEST_F(EventRouterTest, SendEventWithNoSubscribers)
{
    // Sending an event with no subscribers works as expected.

    constexpr int kSendingModule = MockOptions::Module::A;

    ErEvent_t event = {
        .m_type           = ER_EVENT_TYPE__FIRST,
        .m_sending_module = &MockModule<kSendingModule>::m_module,
    };

    ErSend(&event);

    PrepareToDeliverEvents();
    EXPECT_TRUE(MaybeDeliverEvent());

    // When configured for a single-task, all backends return events to their
    // sender immediately after delivering them to all subscribers.

    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event);
}

TEST_F(EventRouterTest, DeliverEventToOneSubscriber)
{
    // Sending an event to a single subscriber works as expected.

    constexpr int kSendingModule     = MockOptions::Module::A;
    constexpr int kSubscribingModule = MockOptions::Module::B;

    ErEvent_t event = {
        .m_type           = ER_EVENT_TYPE__FIRST,
        .m_sending_module = &MockModule<kSendingModule>::m_module,
    };

    ErSubscribe(&MockModule<kSubscribingModule>::m_module, event.m_type);
    ErSend(&event);

    PrepareToDeliverEvents();
    EXPECT_TRUE(MaybeDeliverEvent());

    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event);
}

TEST_F(EventRouterTest, DeliverEventToMultipleSubscribers)
{
    // Multiple modules can subscribe to an event type.

    constexpr int kSendingModule      = MockOptions::Module::A;
    constexpr int kSubscribingModule1 = MockOptions::Module::B;
    constexpr int kSubscribingModule2 = MockOptions::Module::C;

    ErEvent_t event = {
        .m_type           = ER_EVENT_TYPE__FIRST,
        .m_sending_module = &MockModule<kSendingModule>::m_module,
    };

    ErSubscribe(&MockModule<kSubscribingModule1>::m_module, event.m_type);
    ErSubscribe(&MockModule<kSubscribingModule2>::m_module, event.m_type);
    ErSend(&event);

    PrepareToDeliverEvents();
    EXPECT_TRUE(MaybeDeliverEvent());

    EXPECT_EQ(MockModule<kSubscribingModule1>::m_last_event_handled, &event);
    EXPECT_EQ(MockModule<kSubscribingModule2>::m_last_event_handled, &event);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event);
}

TEST_F(EventRouterTest, DontDeliverEventsIfClientsUnsubscribeWhileInTransit)
{
    /// Demonstrate an example of instantaneous unsubscription.

    constexpr int kSendingModule      = MockOptions::Module::A;
    constexpr int kSubscribingModule1 = MockOptions::Module::B;
    constexpr int kSubscribingModule2 = MockOptions::Module::C;

    ErEvent_t event = {
        .m_type           = ER_EVENT_TYPE__FIRST,
        .m_sending_module = &MockModule<kSendingModule>::m_module,
    };

    ErSubscribe(&MockModule<kSubscribingModule1>::m_module, event.m_type);
    ErSubscribe(&MockModule<kSubscribingModule2>::m_module, event.m_type);
    ErSend(&event);

    PrepareToDeliverEvents();
    ErUnsubscribe(&MockModule<kSubscribingModule2>::m_module, event.m_type);
    EXPECT_TRUE(MaybeDeliverEvent());

    EXPECT_EQ(MockModule<kSubscribingModule1>::m_last_event_handled, &event);
    EXPECT_EQ(MockModule<kSubscribingModule2>::m_last_event_handled, nullptr);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event);
}

TEST_F(EventRouterTest, CrossSendAndSubscribe)
{
    /// Two modules send an event and subscribe to each others'.

    constexpr int kModuleA = MockOptions::Module::A;
    constexpr int kModuleB = MockOptions::Module::B;

    ErEvent_t eventa = {.m_type           = ER_EVENT_TYPE__FIRST,
                        .m_sending_module = &MockModule<kModuleA>::m_module};

    ErEvent_t eventb = {.m_type           = ER_EVENT_TYPE__LAST,
                        .m_sending_module = &MockModule<kModuleB>::m_module};

    /// Subscribe the modules to the other modules' events.
    ErSubscribe(&MockModule<kModuleA>::m_module, eventb.m_type);
    ErSubscribe(&MockModule<kModuleB>::m_module, eventa.m_type);

    ErSend(&eventa);
    ErSend(&eventb);

    PrepareToDeliverEvents();
    EXPECT_TRUE(MaybeDeliverEvent());

    EXPECT_EQ(MockModule<kModuleB>::m_last_event_handled, &eventa);  // Deliver.
    EXPECT_EQ(MockModule<kModuleA>::m_last_event_handled, &eventa);  // Return.

    EXPECT_TRUE(MaybeDeliverEvent());

    EXPECT_EQ(MockModule<kModuleA>::m_last_event_handled, &eventb);  // Deliver.
    EXPECT_EQ(MockModule<kModuleB>::m_last_event_handled, &eventb);  // Return.
}

}  // namespace testing
