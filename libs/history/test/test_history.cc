#include <gtest/gtest.h>

#include "hypha/history.h"

class HistoryTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  HistoryTest() = default;
  ~HistoryTest() override = default;
};

TEST_F(HistoryTest, Test) {
  ASSERT_TRUE(true);
}
