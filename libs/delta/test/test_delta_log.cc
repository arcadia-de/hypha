#include <gtest/gtest.h>

#include "hypha/delta.h"
#include "hypha/delta_log.h"

class DeltaLogTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  DeltaLogTest() = default;
  ~DeltaLogTest() override = default;
};

TEST_F(DeltaLogTest, Test_Init) {
  static const size_t kInitCap = 10;
  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, kInitCap);
  ASSERT_NE(log.data, nullptr);
}

TEST_F(DeltaLogTest, Test_Appenddata) {
  static const size_t kInitCap = 5;
  static const size_t kNumInitdata = 3;

  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_cap, kInitCap);
  ASSERT_EQ(log.data_len, 0);

  for (size_t i = 0; i < kNumInitdata; i++) {
    Delta* delta = NewDelta(&log);
    ASSERT_NE(delta, nullptr);
    ASSERT_NE(log.data, nullptr);
    ASSERT_EQ(log.data_len, i + 1);
    ASSERT_EQ(log.data_cap, kInitCap);
  }

  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_len, kNumInitdata);
  ASSERT_EQ(log.data_cap, kInitCap);
}

TEST_F(DeltaLogTest, Test_Appenddata_Grow) {
  static const size_t kInitCap = 2;
  static const size_t kNumInitdata = 3;

  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_cap, kInitCap);
  ASSERT_EQ(log.data_len, 0);

  for (size_t i = 0; i < kNumInitdata; i++) {
    Delta* delta = NewDelta(&log);
    ASSERT_NE(delta, nullptr);
    ASSERT_NE(log.data, nullptr);
    ASSERT_EQ(log.data_len, i + 1);
  }

  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_len, kNumInitdata);
  ASSERT_EQ(log.data_cap, (kInitCap + kNumInitdata) - 1);
}

TEST_F(DeltaLogTest, Test_Free_NoInit) {
  DeltaLog log;
  memset(&log, 0, sizeof(DeltaLog));
  ASSERT_EQ(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, 0);

  FreeDeltaLog(&log);
  ASSERT_EQ(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, 0);
}

TEST_F(DeltaLogTest, Test_Free_InitButEmpty) {
  static const size_t kInitCap = 10;

  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_NE(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, kInitCap);

  FreeDeltaLog(&log);
  ASSERT_EQ(log.data, nullptr);
  ASSERT_EQ(log.data_len, 0);
  ASSERT_EQ(log.data_cap, 0);
}
