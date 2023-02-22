#include "eventrouter.h"

#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

#include "mock_module.h"
#include "mock_options.h"

namespace testing
{

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
        // Undelivered events are an error as far as tests are concerned.
        EXPECT_EQ(ErGetEventToDeliver(), nullptr);
    }

    MockOptions m_options;
};

TEST_F(EventRouterTest, SendingNoEvents)
{
    ErNewLoop();
    ASSERT_EQ(ErGetEventToDeliver(), nullptr);
}

}  // namespace testing
