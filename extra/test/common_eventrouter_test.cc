#include "eventrouter.h"

#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

#include "mock_module.h"

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

    ErTask_t m_task{
        .m_modules     = m_modules,
        .m_num_modules = kNumModules,
#ifdef ER_FREERTOS
        .m_event_queue = (QueueHandle_t)1,
        .m_task_handle = (TaskHandle_t)1,
#endif
    };

    ErOptions_t m_options{
        .m_tasks     = &m_task,
        .m_num_tasks = 1,
        .m_IsInIsr   = IsInIsr,
    };
};

bool MockOptions::m_is_in_isr = false;

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

//==============================================================================
// Tests which assume initialization has completed successfully.
//==============================================================================

class EventRouterTest : public Test
{
   protected:
    EventRouterTest() { ErInit(&m_options.m_options); }
    ~EventRouterTest()
    {
        ErDeinit();
    }

    MockOptions m_options;
};

}  // namespace testing
