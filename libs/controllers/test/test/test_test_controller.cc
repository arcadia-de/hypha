#include <gtest/gtest.h>

#include <cstring>

#include "hypha/resource.h"
#include "hypha/test_controller.h"

class TestControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewTestController();
    ASSERT_NE(ctrl, nullptr);
  }
};

Controller* TestControllerTest::ctrl = nullptr;

// Test is a synthetic, debug-only resource kind with no real external state to check --
// Status should always report kStatusOk regardless of the resource passed in.
TEST_F(TestControllerTest, StatusIsAlwaysOk) {
  Resource res = {};
  res.info.name = strdup("test-resource");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TestControllerTest, DiffIsAlwaysOk) {
  Resource res = {};
  res.info.name = strdup("test-resource");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusOk);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "no real external state"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
