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

TEST_F(SymlinkControllerTest, DiffFailsWhenTargetMissing) {
  const std::string source = TempPath("-diff-source1");
  FILE* f = fopen(source.c_str(), "w");
  ASSERT_NE(f, nullptr);
  fclose(f);

  const std::string target = TempPath("-diff-missing-link");
  Resource res = MakeDesiredResource(source, target);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "does not exist"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
}

TEST_F(SymlinkControllerTest, DiffPassesWhenLinkPointsToExpectedSource) {
  const std::string source = TempPath("-diff-source2");
  FILE* f = fopen(source.c_str(), "w");
  ASSERT_NE(f, nullptr);
  fclose(f);

  const std::string target = TempPath("-diff-correct-link");
  Resource res = MakeDesiredResource(source, target);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusOk);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
  remove(target.c_str());
}

// This is the case that actually distinguishes Diff from Status: a target that IS a symlink
// (so Status reports kStatusOk) but points somewhere other than the expected source.
TEST_F(SymlinkControllerTest, DiffFailsWhenLinkPointsElsewhereEvenThoughStatusPasses) {
  const std::string source = TempPath("-diff-source3");
  FILE* sf = fopen(source.c_str(), "w");
  ASSERT_NE(sf, nullptr);
  fclose(sf);

  const std::string wrong_source = TempPath("-diff-wrong-source3");
  FILE* wf = fopen(wrong_source.c_str(), "w");
  ASSERT_NE(wf, nullptr);
  fclose(wf);

  const std::string target = TempPath("-diff-wrong-link");
  ASSERT_EQ(symlink(wrong_source.c_str(), target.c_str()), 0);

  Resource res = MakeDesiredResource(source, target);
  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk) << "Status only checks that target IS a symlink";

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  // NOTE: Reason is a fixed HYPHA_REASON_MAX_LENGTH (128) byte buffer, shared with Plan/
  // Validate's messages -- cramming target + actual + expected temp-dir paths into one
  // message can legitimately truncate before reaching the third one, so only assert on
  // what's guaranteed to fit first.
  EXPECT_NE(strstr(dlog.data[0].reason, wrong_source.c_str()), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
  remove(wrong_source.c_str());
  remove(target.c_str());
}
