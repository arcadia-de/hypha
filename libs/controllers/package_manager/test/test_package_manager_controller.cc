#include <cstring>
#include <gtest/gtest.h>
#include <string>

#include "hypha/annotation.h"
#include "hypha/package_manager.h"
#include "hypha/package_manager_controller.h"
#include "hypha/resource.h"
#include "hypha/resource_bootstrap.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

namespace {

static PackageStatus MockStatus(PackageManager*, const char*, void*) {
  return kPackageInstalled;
}

Resource MakeDesiredResource(const std::string& type) {
  Resource res = {};
  res.info.name = strdup("test-package-manager");
  const std::string raw = "{\"type\": \"" + type + "\"}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class PackageManagerControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewPackageManagerController();
    ASSERT_NE(ctrl, nullptr);

    // Register a mock backend so this doesn't depend on any real package manager binary
    // being present on the machine running the tests. "/bin/true" always exists and is
    // executable, standing in for a real backend's binary for the access(X_OK) check.
    static const PackageManagerConfig kMockConfig = {
        .install = nullptr,
        .status = MockStatus,
        .uninstall = nullptr,
    };
    ASSERT_NE(NewPackageManager("TestPkgManager", "/bin/true", &kMockConfig, nullptr, nullptr), nullptr);
  }
};

Controller* PackageManagerControllerTest::ctrl = nullptr;

TEST_F(PackageManagerControllerTest, ValidateFailsWithoutTypeField) {
  Resource res = {};
  res.info.name = strdup("bad-package-manager");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(PackageManagerControllerTest, ValidateFailsForUnregisteredType) {
  Resource res = MakeDesiredResource("NotARealPackageManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_FALSE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  EXPECT_EQ(ControllerNormalize(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(PackageManagerControllerTest, NormalizeStampsProvidesAnnotation) {
  Resource res = MakeDesiredResource("TestPkgManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_TRUE(ControllerValidate(ctrl, &res, &vlog));
  FreeValidationLog(&vlog, 4);

  ASSERT_EQ(ControllerNormalize(ctrl, &res), kStatusOk);

  AnnotationValue expected;
  memset(expected, 0, sizeof(AnnotationValue));
  strncpy(expected, "TestPkgManager", sizeof(AnnotationValue) - 1);
  EXPECT_TRUE(ResourceHasAnnotationV(&res, &expected))
      << "expected `hypha/provides` annotation with value TestPkgManager after Normalize";

  // Normalize should be idempotent -- running it again shouldn't duplicate the annotation.
  ASSERT_EQ(ControllerNormalize(ctrl, &res), kStatusOk);
  EXPECT_EQ(res.info.annotations_len, 1u);

  Plan plan = {};
  InitPlan(&plan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &plan), kNoAction);
  FreePlan(&plan);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  free(res.info.annotations);
}

TEST_F(PackageManagerControllerTest, StatusFailsForUnregisteredType) {
  Resource res = MakeDesiredResource("NotARealPackageManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

// Diff and Status agree here -- there's no more granular comparison available than "is this
// backend registered and its binary reachable."
TEST_F(PackageManagerControllerTest, DiffMatchesStatus) {
  Resource res = MakeDesiredResource("TestPkgManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusOk);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "is available"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(PackageManagerControllerTest, DiffFailsForUnregisteredType) {
  Resource res = MakeDesiredResource("NotARealPackageManager");

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "not a registered package manager"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
