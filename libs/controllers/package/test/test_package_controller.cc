#include <cstring>
#include <gtest/gtest.h>
#include <string>

#include "hypha/action_log.h"
#include "hypha/package_controller.h"
#include "hypha/package_manager.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

namespace {

bool g_mock_installed = false;

PackageStatus MockInstall(PackageManager*, const char*, void*) {
  g_mock_installed = true;
  return kPackageInstalled;
}

PackageStatus MockStatus(PackageManager*, const char*, void*) {
  return g_mock_installed ? kPackageInstalled : kPackageUninstalled;
}

Resource MakeDesiredResource(const std::string& resource_name, const char* name_field, const std::string& manager) {
  Resource res = {};
  res.info.name = strdup(resource_name.c_str());

  std::string raw = "{";
  if (name_field)
    raw += "\"name\": \"" + std::string(name_field) + "\", ";
  raw += "\"manager\": \"" + manager + "\"}";

  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class PackageControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewPackageController();
    ASSERT_NE(ctrl, nullptr);

    static const PackageManagerConfig kMockConfig = {
        .install = MockInstall,
        .status = MockStatus,
        .uninstall = nullptr,
    };
    ASSERT_NE(NewPackageManager("TestPkgManager", "/bin/true", &kMockConfig, nullptr, nullptr), nullptr);
  }

  void SetUp() override {
    g_mock_installed = false;
  }
};

Controller* PackageControllerTest::ctrl = nullptr;

TEST_F(PackageControllerTest, ValidateFailsWithoutManagerField) {
  Resource res = {};
  res.info.name = strdup("bad-package");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(PackageControllerTest, ValidateFailsForUnregisteredManager) {
  Resource res = MakeDesiredResource("cowsay-pkg", "cowsay", "NotARealPackageManager");

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

TEST_F(PackageControllerTest, ObservePlanApplyInstallsPackageAndBecomesIdempotent) {
  Resource res = MakeDesiredResource("cowsay-pkg", "cowsay", "TestPkgManager");

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

  EXPECT_FALSE(g_mock_installed);
  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);
  EXPECT_TRUE(g_mock_installed);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  // Re-planning against an already-installed package should be a no-op.
  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(PackageControllerTest, ObserveDefaultsPackageNameToResourceNameWhenFieldMissing) {
  Resource res = MakeDesiredResource("cowsay", nullptr, "TestPkgManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_TRUE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  PackageManager* mgr = FindPackageManager("TestPkgManager");
  ASSERT_NE(mgr, nullptr);
  EXPECT_EQ(PackageManagerStatus(mgr, res.info.name), kPackageInstalled);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(PackageControllerTest, StatusFailsWhenNotInstalled) {
  Resource res = MakeDesiredResource("cowsay-pkg", "cowsay", "TestPkgManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

// Diff and Status agree here -- there's no more granular comparison available than
// "does the manager report this installed right now."
TEST_F(PackageControllerTest, DiffMatchesStatus) {
  Resource res = MakeDesiredResource("cowsay-pkg", "cowsay", "TestPkgManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "not installed"), nullptr);
  FreeDeltaLog(&dlog);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  DeltaLog dlog2 = {};
  InitDeltaLog(&dlog2, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog2), kStatusOk);
  ASSERT_EQ(dlog2.data_len, 1u);
  EXPECT_NE(strstr(dlog2.data[0].reason, "is installed"), nullptr);
  FreeDeltaLog(&dlog2);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
