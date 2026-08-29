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
  ASSERT_EQ(log.deltas_len, 0);
  ASSERT_EQ(log.deltas_cap, kInitCap);
  ASSERT_NE(log.deltas, nullptr);
}

TEST_F(DeltaLogTest, Test_AppendDeltas) {
  static const size_t kInitCap = 5;
  static const size_t kNumInitDeltas = 3;

  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_NE(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_cap, kInitCap);
  ASSERT_EQ(log.deltas_len, 0);

  for (size_t i = 0; i < kNumInitDeltas; i++) {
    Delta* delta = NewDelta(&log);
    ASSERT_NE(delta, nullptr);
    ASSERT_NE(log.deltas, nullptr);
    ASSERT_EQ(log.deltas_len, i + 1);
    ASSERT_EQ(log.deltas_cap, kInitCap);
  }

  ASSERT_NE(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_len, kNumInitDeltas);
  ASSERT_EQ(log.deltas_cap, kInitCap);
}

TEST_F(DeltaLogTest, Test_AppendDeltas_Grow) {
  static const size_t kInitCap = 2;
  static const size_t kNumInitDeltas = 3;

  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_NE(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_cap, kInitCap);
  ASSERT_EQ(log.deltas_len, 0);

  for (size_t i = 0; i < kNumInitDeltas; i++) {
    Delta* delta = NewDelta(&log);
    ASSERT_NE(delta, nullptr);
    ASSERT_NE(log.deltas, nullptr);
    ASSERT_EQ(log.deltas_len, i + 1);
  }

  ASSERT_NE(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_len, kNumInitDeltas);
  ASSERT_EQ(log.deltas_cap, (kInitCap + kNumInitDeltas) - 1);
}

TEST_F(DeltaLogTest, Test_Free_NoInit) {
  DeltaLog log;
  memset(&log, 0, sizeof(DeltaLog));
  ASSERT_EQ(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_len, 0);
  ASSERT_EQ(log.deltas_cap, 0);

  FreeDeltaLog(&log);
  ASSERT_EQ(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_len, 0);
  ASSERT_EQ(log.deltas_cap, 0);
}

TEST_F(DeltaLogTest, Test_Free_InitButEmpty) {
  static const size_t kInitCap = 10;

  DeltaLog log;
  InitDeltaLog(&log, kInitCap);
  ASSERT_NE(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_len, 0);
  ASSERT_EQ(log.deltas_cap, kInitCap);

  FreeDeltaLog(&log);
  ASSERT_EQ(log.deltas, nullptr);
  ASSERT_EQ(log.deltas_len, 0);
  ASSERT_EQ(log.deltas_cap, 0);
}
