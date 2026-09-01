#include <gtest/gtest.h>

#include "hypha/planner.h"

class QueryTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  QueryTest() = default;
  ~QueryTest() override = default;
};

TEST_F(QueryTest, Test) {
  ASSERT_TRUE(true);
}

// Regression test: Plan is always caller-owned (embedded in a longer-lived struct, e.g.
// `orc->plan`), never allocated via a `NewPlan()`. FreePlan used to also `free(pl)` itself,
// which crashes (invalid pointer) the instant it's called on stack- or embedded-storage --
// i.e. every real call site. This exercises Init -> use -> Free on stack storage directly.
TEST_F(QueryTest, Test_FreePlanDoesNotFreeCallerOwnedStorage) {
  Plan plan = {};
  InitPlan(&plan, 4);
  ASSERT_NE(plan.actions, nullptr);
  ASSERT_EQ(plan.actions_len, 0u);
  ASSERT_EQ(plan.actions_cap, 4u);

  PlannedAction* action = NewPlannedAction(&plan);
  ASSERT_NE(action, nullptr);
  ASSERT_EQ(plan.actions_len, 1u);

  FreePlan(&plan);
  EXPECT_EQ(plan.actions, nullptr);
  EXPECT_EQ(plan.actions_len, 0u);
  EXPECT_EQ(plan.actions_cap, 0u);

  // Safe to re-init and reuse the same storage after freeing.
  InitPlan(&plan, 2);
  EXPECT_NE(plan.actions, nullptr);
  FreePlan(&plan);
}
