#include <git2.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "hypha/action_log.h"
#include "hypha/planner.h"
#include "hypha/repository_controller.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

namespace {

Resource MakeDesiredResource(const std::string& url, const std::string& destination) {
  Resource res = {};
  res.info.name = strdup("test-repository");
  const std::string raw = "{\"url\": \"" + url + "\", \"destination\": \"" + destination + "\"}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class RepositoryControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;
  std::string source_repo;

  static void SetUpTestSuite() {
    ctrl = NewRepositoryController();
    ASSERT_NE(ctrl, nullptr);
    // Controller.init runs git_libgit2_init(); the orchestrator normally calls this via
    // ControllerInit(ctrl), which we mirror here so libgit2 is ready for every test.
    ControllerInit(ctrl);
  }

  static void TearDownTestSuite() {
    ControllerDeInit(ctrl);
  }

  void SetUp() override {
    // A local, on-disk repository stands in for a remote -- git clone treats a filesystem
    // path as a valid transport, so this exercises the real Clone() path without network.
    source_repo = std::string(::testing::TempDir()) + "repository-controller-test-src-" +
                  std::to_string(getpid());
    git_repository* repo = nullptr;
    ASSERT_EQ(git_repository_init(&repo, source_repo.c_str(), false), 0);
    git_repository_free(repo);
  }

  std::string TempPath(const char* suffix) const {
    return std::string(::testing::TempDir()) + "repository-controller-test-" + std::to_string(getpid()) + suffix;
  }
};

Controller* RepositoryControllerTest::ctrl = nullptr;

TEST_F(RepositoryControllerTest, ValidateFailsWithoutUrlOrDestination) {
  Resource res = {};
  res.info.name = strdup("bad-repository");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(RepositoryControllerTest, ObservePlanApplyClonesRepoAndBecomesIdempotent) {
  const std::string destination = TempPath("-dest");
  Resource res = MakeDesiredResource(source_repo, destination);

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

  struct stat dest_stat;
  ASSERT_EQ(stat((destination + "/.git").c_str(), &dest_stat), 0) << "expected clone to create a .git directory";

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  // Re-planning against an already-cloned destination should be a no-op.
  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(RepositoryControllerTest, StatusFailsWhenDestinationMissing) {
  const std::string destination = TempPath("-never-cloned");
  Resource res = MakeDesiredResource(source_repo, destination);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
