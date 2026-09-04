#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>

#include "hypha/action_log.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/template_controller.h"
#include "hypha/validation_log.h"

extern "C" char* RenderTemplate(char* tpl, char* data, bool is_yaml) {
  (void)is_yaml;
  std::string out = tpl ? tpl : "";
  if (data)
    out += std::string(" ") + data;
  return strdup(out.c_str());
}

namespace {

Resource MakeDesiredResource(const std::string& target, const std::string& extra_fields) {
  Resource res = {};
  res.info.name = strdup("test-template");
  const std::string raw = "{\"target\": \"" + target + "\"" + extra_fields + "}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class TemplateControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewTemplateController();
    ASSERT_NE(ctrl, nullptr);
  }

  std::string TempPath(const char* suffix) const {
    return std::string(::testing::TempDir()) + "template-controller-test-" + std::to_string(getpid()) + suffix;
  }
};

Controller* TemplateControllerTest::ctrl = nullptr;

TEST_F(TemplateControllerTest, ValidateFailsWithoutTargetField) {
  Resource res = {};
  res.info.name = strdup("bad-template");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TemplateControllerTest, ValidateFailsWithoutTemplateOrTemplateFile) {
  const std::string target = TempPath("-dest1");
  Resource res = MakeDesiredResource(target, "");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_FALSE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TemplateControllerTest, ValidateFailsWhenTemplateFileDoesNotExist) {
  const std::string target = TempPath("-dest2");
  Resource res = MakeDesiredResource(target, ", \"templateFile\": \"/nonexistent/template.tpl\"");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_FALSE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TemplateControllerTest, InlineTemplateRendersAndBecomesIdempotent) {
  const std::string target = TempPath("-inline-dest");
  Resource res = MakeDesiredResource(target, ", \"template\": \"hello\"");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_TRUE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  EXPECT_EQ(action, kUpdateAction);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  FILE* f = fopen(target.c_str(), "r");
  ASSERT_NE(f, nullptr);
  char buf[32] = {0};
  fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  EXPECT_STREQ(buf, "hello");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(target.c_str());
}

TEST_F(TemplateControllerTest, TemplateFileRendersFileContents) {
  const std::string source = TempPath("-source.tpl");
  FILE* sf = fopen(source.c_str(), "w");
  ASSERT_NE(sf, nullptr);
  fputs("from-file", sf);
  fclose(sf);

  const std::string target = TempPath("-templatefile-dest");
  Resource res = MakeDesiredResource(target, ", \"templateFile\": \"" + source + "\"");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_TRUE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  EXPECT_EQ(action, kUpdateAction);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  FILE* f = fopen(target.c_str(), "r");
  ASSERT_NE(f, nullptr);
  char buf[32] = {0};
  fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  EXPECT_STREQ(buf, "from-file");

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
  remove(target.c_str());
}

TEST_F(TemplateControllerTest, StatusFailsWhenTargetMissing) {
  const std::string target = TempPath("-never-rendered");
  Resource res = MakeDesiredResource(target, ", \"template\": \"hello\"");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(TemplateControllerTest, DiffFailsWhenTargetMissing) {
  const std::string target = TempPath("-diff-never-rendered");
  Resource res = MakeDesiredResource(target, ", \"template\": \"hello\"");

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
}

TEST_F(TemplateControllerTest, DiffPassesWhenContentMatches) {
  const std::string target = TempPath("-diff-match-dest");
  Resource res = MakeDesiredResource(target, ", \"template\": \"hello\"");

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
  remove(target.c_str());
}

// This is the case that actually distinguishes Diff from Status: a target that exists (so
// Status reports kStatusOk) but whose content doesn't match what would actually be rendered.
TEST_F(TemplateControllerTest, DiffFailsWhenContentDiffersEvenThoughStatusPasses) {
  const std::string target = TempPath("-diff-stale-dest");
  FILE* f = fopen(target.c_str(), "w");
  ASSERT_NE(f, nullptr);
  fputs("stale, unrelated content", f);
  fclose(f);

  Resource res = MakeDesiredResource(target, ", \"template\": \"hello\"");
  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk) << "Status only checks that target exists";

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "bytes"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(target.c_str());
}
