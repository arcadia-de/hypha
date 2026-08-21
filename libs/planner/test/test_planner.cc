#include <gtest/gtest.h>

#include "hypha/planner.h"

class PlannerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  PlannerTest() = default;
  ~PlannerTest() override = default;
};

TEST_F(PlannerTest, Test) {
  ASSERT_TRUE(true);
}
