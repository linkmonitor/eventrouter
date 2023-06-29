#include "eventrouter.h"

#include "gtest/gtest.h"
#include "mock_module.h"

namespace
{

struct MockOptions
{
   public:
    struct Module
    {
        static constexpr int A = 0;
        static constexpr int B = 1;
        static constexpr int C = 2;
        static constexpr int D = 3;
    };

    MockOptions()
    {
        MockModule<Module::A>::Reset();
        MockModule<Module::B>::Reset();
        MockModule<Module::C>::Reset();
        MockModule<Module::D>::Reset();
    }

    const ErTask_t m_tasks[1] = {
        {
            .m_modules =
                (ErModule_t*[]){
                    &MockModule<Module::A>::m_module,
                    &MockModule<Module::B>::m_module,
                    &MockModule<Module::C>::m_module,
                    &MockModule<Module::D>::m_module,
                },
            .m_num_modules = 4,
        },
    };
    const ErOptions_t m_options{
        .m_tasks     = m_tasks,
        .m_num_tasks = 1,
    };
};

}  // namespace

namespace testing
{

class ErBaremetalTest : public Test
{
   protected:
    ErBaremetalTest() { ErInit(&m_options.m_options); }
    ~ErBaremetalTest()
    {
        ErNewLoop();
        assert(ErGetEventToDeliver() == nullptr);
        ErDeinit();
    }

    MockOptions m_options;
};

TEST_F(ErBaremetalTest, SendEventAndReuse)
{
    constexpr int kSendingModule     = MockOptions::Module::A;
    constexpr int kSubscribingModule = MockOptions::Module::B;

    ErEvent_t event = {
        .m_type            = ER_EVENT_TYPE__FIRST,
        .m_reference_count = {0},
        .m_sending_module  = &MockModule<kSendingModule>::m_module,
        .m_next            = {.m_next = NULL},
    };

    ErSubscribe(&MockModule<kSubscribingModule>::m_module, event.m_type);

    MockModule<kSubscribingModule>::m_event_handler_ret =
        ER_EVENT_HANDLER_RET__HANDLED;

    ErSend(&event);

    ErNewLoop();
    EXPECT_EQ(ErGetEventToDeliver(), &event);
    ErCallHandlers(&event);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event);
    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event);
    EXPECT_EQ(ErGetEventToDeliver(), nullptr);

    ErSend(&event);

    ErNewLoop();
    EXPECT_EQ(ErGetEventToDeliver(), &event);
    ErCallHandlers(&event);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event);
    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event);
    EXPECT_EQ(ErGetEventToDeliver(), nullptr);
}

TEST_F(ErBaremetalTest, SendTwoEventsAndReuse)
{
    constexpr int kSendingModule     = MockOptions::Module::A;
    constexpr int kSubscribingModule = MockOptions::Module::B;

    ErEvent_t event1 = {
        .m_type            = ER_EVENT_TYPE__1,
        .m_reference_count = {0},
        .m_sending_module  = &MockModule<kSendingModule>::m_module,
        .m_next            = {.m_next = NULL},
    };

    ErEvent_t event2 = {
        .m_type            = ER_EVENT_TYPE__2,
        .m_reference_count = {0},
        .m_sending_module  = &MockModule<kSendingModule>::m_module,
        .m_next            = {.m_next = NULL},
    };

    ErSubscribe(&MockModule<kSubscribingModule>::m_module, event1.m_type);
    ErSubscribe(&MockModule<kSubscribingModule>::m_module, event2.m_type);

    MockModule<kSubscribingModule>::m_event_handler_ret =
        ER_EVENT_HANDLER_RET__HANDLED;

    ErSend(&event1);
    ErSend(&event2);

    ErNewLoop();
    EXPECT_EQ(ErGetEventToDeliver(), &event1);
    ErCallHandlers(&event1);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event1);
    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event1);
    EXPECT_EQ(ErGetEventToDeliver(), &event2);
    ErCallHandlers(&event2);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event2);
    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event2);
    EXPECT_EQ(ErGetEventToDeliver(), nullptr);

    ErSend(&event1);
    ErSend(&event2);

    ErNewLoop();
    EXPECT_EQ(ErGetEventToDeliver(), &event1);
    ErCallHandlers(&event1);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event1);
    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event1);
    EXPECT_EQ(ErGetEventToDeliver(), &event2);
    ErCallHandlers(&event2);
    EXPECT_EQ(MockModule<kSendingModule>::m_last_event_handled, &event2);
    EXPECT_EQ(MockModule<kSubscribingModule>::m_last_event_handled, &event2);
    EXPECT_EQ(ErGetEventToDeliver(), nullptr);
}

}  // namespace testing
