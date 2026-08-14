#include <gtest/gtest.h>

#include "hypha/event.h"

class EventTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  EventTest() = default;
  ~EventTest() override = default;
};

TEST_F(EventTest, Test) {
  ASSERT_TRUE(true);
}
