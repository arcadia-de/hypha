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

// Creates an initial commit in `repo` so it has a real HEAD to report -- a freshly
// git_repository_init()'d repo has none yet.
void CreateInitialCommit(git_repository* repo) {
  git_signature* sig = nullptr;
  ASSERT_EQ(git_signature_now(&sig, "Test", "test@example.com"), 0);

  git_index* index = nullptr;
  ASSERT_EQ(git_repository_index(&index, repo), 0);
  git_oid tree_oid;
  ASSERT_EQ(git_index_write_tree(&tree_oid, index), 0);

  git_tree* tree = nullptr;
  ASSERT_EQ(git_tree_lookup(&tree, repo, &tree_oid), 0);

  git_oid commit_oid;
  ASSERT_EQ(git_commit_create_v(&commit_oid, repo, "HEAD", sig, sig, nullptr, "initial commit", tree, 0), 0);

  git_tree_free(tree);
  git_index_free(index);
  git_signature_free(sig);
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

TEST_F(RepositoryControllerTest, DiffFailsWhenDestinationMissing) {
  const std::string destination = TempPath("-diff-never-cloned");
  Resource res = MakeDesiredResource(source_repo, destination);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 1u);
  // NOTE: Reason is a fixed 128-byte buffer; destination + url together can legitimately
  // truncate before "would be cloned there", so only assert on what's guaranteed to fit.
  EXPECT_NE(strstr(dlog.data[0].reason, "is not a git repository"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

// Cloning our `source_repo` fixture (which has no commits) gives a destination with no HEAD
// either -- Diff should recognize that distinctly rather than treating it as an error.
TEST_F(RepositoryControllerTest, DiffReportsNoCommitsYetForEmptyClone) {
  const std::string destination = TempPath("-diff-empty-clone");
  Resource res = MakeDesiredResource(source_repo, destination);

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
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, "no commits yet"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

// This exercises the actual branch/commit reporting path: a source repo *with* a real commit
// to clone and then report on.
TEST_F(RepositoryControllerTest, DiffReportsBranchAndCommitForRealClone) {
  git_repository* src = nullptr;
  ASSERT_EQ(git_repository_open(&src, source_repo.c_str()), 0);
  CreateInitialCommit(src);
  git_repository_free(src);

  const std::string destination = TempPath("-diff-real-clone");
  Resource res = MakeDesiredResource(source_repo, destination);

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
  ASSERT_EQ(dlog.data_len, 1u);
  EXPECT_NE(strstr(dlog.data[0].reason, destination.c_str()), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
