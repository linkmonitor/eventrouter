#include "eventrouter.h"

#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

#include "mock_module.h"
#include "mock_options.h"

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
