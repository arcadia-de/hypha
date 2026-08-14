#include <gtest/gtest.h>

#include "hypha/log.h"

class LogTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  LogTest() = default;
  ~LogTest() override = default;
};

TEST_F(LogTest, Test) {
  ASSERT_TRUE(true);
}
