#include <gtest/gtest.h>

#include "gtest/gtest.h"
#include "hypha/action_log.h"
#include "hypha/assertions.h"
#include "hypha/controller_action.h"
#include "hypha/reason.h"

class AppliedActionLogTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  AppliedActionLogTest() = default;
  ~AppliedActionLogTest() override = default;
};

TEST_F(AppliedActionLogTest, Test_Init) {
  static const size_t kInitCap = 10;
  AppliedActionLog log;
  InitAppliedActionLog(&log, kInitCap);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, kInitCap);
  ASSERT_NE(log.data, nullptr);
}

TEST_F(AppliedActionLogTest, Test_Appenddata) {
  static const size_t kInitCap = 5;
  static const size_t kNumInitdata = 3;

  AppliedActionLog log;
  InitAppliedActionLog(&log, kInitCap);
  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_cap, kInitCap);
  ASSERT_EQ(log.data_len, 0);

  for (size_t i = 0; i < kNumInitdata; i++) {
    AppliedAction* action = NewAppliedAction(&log);
    ASSERT_NE(action, nullptr);
    ASSERT_NE(log.data, nullptr);
    ASSERT_EQ(log.data_len, i + 1);
    ASSERT_EQ(log.data_cap, kInitCap);
  }

  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_len, kNumInitdata);
  ASSERT_EQ(log.data_cap, kInitCap);
}

TEST_F(AppliedActionLogTest, Test_Appenddata_Grow) {
  static const size_t kInitCap = 2;
  static const size_t kNumInitdata = 3;

  AppliedActionLog log;
  InitAppliedActionLog(&log, kInitCap);
  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_cap, kInitCap);
  ASSERT_EQ(log.data_len, 0);

  for (size_t i = 0; i < kNumInitdata; i++) {
    AppliedAction* action = NewAppliedAction(&log);
    ASSERT_NE(action, nullptr);
    ASSERT_NE(log.data, nullptr);
    ASSERT_EQ(log.data_len, i + 1);
  }

  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_len, kNumInitdata);
  ASSERT_EQ(log.data_cap, (kInitCap + kNumInitdata) - 1);
}

TEST_F(AppliedActionLogTest, Test_Free_NoInit) {
  AppliedActionLog log;
  memset(&log, 0, sizeof(AppliedActionLog));
  ASSERT_EQ(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, 0);

  FreeAppliedActionLog(&log);
  ASSERT_EQ(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, 0);
}

TEST_F(AppliedActionLogTest, Test_Free_InitButEmpty) {
  static const size_t kInitCap = 10;

  AppliedActionLog log;
  InitAppliedActionLog(&log, kInitCap);
  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, kInitCap);

  FreeAppliedActionLog(&log);
  ASSERT_EQ(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, 0);
}

static inline auto IsReason(const Reason lhs, const std::string_view rhs) -> testing::AssertionResult {
  if (strncmp(lhs, rhs.data(), HYPHA_REASON_MAX_LENGTH) != 0)
    return ::testing::AssertionFailure() << "Reason " << std::string(lhs, HYPHA_REASON_MAX_LENGTH)
                                         << " does not equal: " << rhs;
  return ::testing::AssertionSuccess();
}

#define DEFINE_NEW_ACTION_TEST(Name)                                             \
  TEST_F(AppliedActionLogTest, Test_New##Name##Action) {                         \
    static const size_t kInitCap = 10;                                           \
    AppliedActionLog log;                                                        \
    InitAppliedActionLog(&log, kInitCap);                                        \
    const auto action = New##Name##Action(&log, nullptr, "no action specified"); \
    ASSERT_NE(action, nullptr);                                                  \
    ASSERT_EQ(action->resource, nullptr);                                        \
    ASSERT_EQ(action->action, k##Name##Action);                                  \
    ASSERT_NE(action->timestamp.tv_sec, 0);                                      \
    ASSERT_NE(action->timestamp.tv_nsec, 0);                                     \
    ASSERT_TRUE(IsReason(action->reason, "no action specified"));                \
    ASSERT_NE(log.data, nullptr);                                                \
    ASSERT_EQ(log.data_len, 1);                                                  \
    ASSERT_EQ(log.data_cap, kInitCap);                                           \
  }

FOR_EACH_CONTROLLER_ACTION(DEFINE_NEW_ACTION_TEST);
#undef DEFINE_NEW_ACTION_TEST
