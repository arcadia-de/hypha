#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha/action_log.h"
#include "hypha/directory_controller.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

namespace {

Resource MakeDesiredResource(const std::string& target) {
  Resource res = {};
  res.info.name = strdup("test-directory");
  const std::string raw = "{\"target\": \"" + target + "\"}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class DirectoryControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewDirectoryController();
    ASSERT_NE(ctrl, nullptr);
  }

  std::string TempPath(const char* suffix) const {
    return std::string(::testing::TempDir()) + "directory-controller-test-" + std::to_string(getpid()) + suffix;
  }
};

Controller* DirectoryControllerTest::ctrl = nullptr;

TEST_F(DirectoryControllerTest, ValidateFailsWithoutTargetField) {
  Resource res = {};
  res.info.name = strdup("bad-directory");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(DirectoryControllerTest, ObservePlanApplyCreatesNestedDirectoryAndBecomesIdempotent) {
  const std::string target = TempPath("-nested/a/b");
  Resource res = MakeDesiredResource(target);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_TRUE(ControllerValidate(ctrl, &res, &vlog));
  EXPECT_GT(vlog.results_len, 0u);
  FreeValidationLog(&vlog, 4);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  EXPECT_EQ(action, kCreateAction);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  struct stat st;
  ASSERT_EQ(stat(target.c_str(), &st), 0);
  EXPECT_TRUE(S_ISDIR(st.st_mode));

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(DirectoryControllerTest, StatusFailsWhenTargetMissing) {
  const std::string target = TempPath("-never-created");
  Resource res = MakeDesiredResource(target);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(DirectoryControllerTest, PlanIsNoOpWhenTargetExistsButIsNotADirectory) {
  const std::string target = TempPath("-a-file");
  FILE* f = fopen(target.c_str(), "w");
  ASSERT_NE(f, nullptr);
  fclose(f);

  Resource res = MakeDesiredResource(target);
  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  Plan plan = {};
  InitPlan(&plan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &plan), kNoAction);
  FreePlan(&plan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(target.c_str());
}
