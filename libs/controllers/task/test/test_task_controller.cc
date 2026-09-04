#include <gtest/gtest.h>

#include <cstring>

#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/task_controller.h"

namespace {

Resource MakeResource(const char* spec_json) {
  Resource res = {};
  res.info.name = strdup("test-task");
  res.spec.raw = strdup(spec_json);
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class TaskControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewTaskController();
    ASSERT_NE(ctrl, nullptr);
  }
};

Controller* TaskControllerTest::ctrl = nullptr;

// A Task with no `check` command has no generic, external way for Status to verify its
// effect -- that's exactly why `check` is opt-in. kStatusOk here means "nothing to check,"
// which is the correct, honest default rather than treating an unverifiable Task as failed.
TEST_F(TaskControllerTest, StatusIsOkWhenNoCheckIsSpecified) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/true"]}})");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TaskControllerTest, StatusPassesWhenCheckExitsZero) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/true"]}, "check": {"command": ["/usr/bin/true"]}})");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TaskControllerTest, StatusFailsWhenCheckExitsNonZero) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/true"]}, "check": {"command": ["/usr/bin/false"]}})");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

// Status doesn't depend on Plan having run first in the same process -- it parses its own
// TaskSpec straight from the resource, not the thread_local state Plan populates. This
// exercises Status in isolation, with no preceding ControllerPlan call at all.
TEST_F(TaskControllerTest, StatusWorksWithoutAPriorPlanCall) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/echo", "hi"]}, "check": {"command": ["/usr/bin/true"]}})");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

// Diff and Status ask the same question here -- "does the check command currently pass" --
// so they share one implementation.
TEST_F(TaskControllerTest, DiffMatchesStatusWhenNoCheckIsSpecified) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/true"]}})");

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusOk);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "nothing to compare"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TaskControllerTest, DiffPassesWhenCheckExitsZero) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/true"]}, "check": {"command": ["/usr/bin/true"]}})");

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusOk);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "check passed"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TaskControllerTest, DiffFailsWhenCheckExitsNonZero) {
  Resource res = MakeResource(R"({"exec": {"command": ["/usr/bin/true"]}, "check": {"command": ["/usr/bin/false"]}})");

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "exited with code"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
