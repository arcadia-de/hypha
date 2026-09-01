#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha/action_log.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/symlink_controller.h"
#include "hypha/validation_log.h"

namespace {

Resource MakeDesiredResource(const std::string& source, const std::string& target) {
  Resource res = {};
  res.info.name = strdup("test-symlink");
  const std::string raw = "{\"source\": \"" + source + "\", \"target\": \"" + target + "\"}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class SymlinkControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewSymlinkController();
    ASSERT_NE(ctrl, nullptr);
  }

  std::string TempPath(const char* suffix) const {
    return std::string(::testing::TempDir()) + "symlink-controller-test-" + std::to_string(getpid()) + suffix;
  }
};

Controller* SymlinkControllerTest::ctrl = nullptr;

TEST_F(SymlinkControllerTest, ObserveFailsGracefullyWithoutSourceOrTargetField) {
  Resource res = {};
  res.info.name = strdup("bad-symlink");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  EXPECT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(SymlinkControllerTest, ObserveFailsGracefullyWithOnlySourceField) {
  Resource res = {};
  res.info.name = strdup("bad-symlink-2");
  res.spec.raw = strdup("{\"source\": \"/etc/hosts\"}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  EXPECT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(SymlinkControllerTest, ObservePlanApplyCreatesSymlinkAndBecomesIdempotent) {
  const std::string source = TempPath("-source");
  FILE* f = fopen(source.c_str(), "w");
  ASSERT_NE(f, nullptr);
  fputs("hello", f);
  fclose(f);

  const std::string target = TempPath("-link");
  Resource res = MakeDesiredResource(source, target);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_TRUE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  EXPECT_EQ(action, kCreateAction);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  struct stat lst;
  ASSERT_EQ(lstat(target.c_str(), &lst), 0);
  EXPECT_TRUE(S_ISLNK(lst.st_mode));

  char buf[PATH_MAX] = {0};
  ssize_t n = readlink(target.c_str(), buf, sizeof(buf) - 1);
  ASSERT_GT(n, 0);
  EXPECT_STREQ(buf, source.c_str());

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
  remove(target.c_str());
}

TEST_F(SymlinkControllerTest, StatusFailsWhenSourceMissing) {
  const std::string source = TempPath("-never-existed");
  const std::string target = TempPath("-orphan-link");
  Resource res = MakeDesiredResource(source, target);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
