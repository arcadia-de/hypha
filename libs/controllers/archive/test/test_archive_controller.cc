#include <archive.h>
#include <archive_entry.h>
#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>

#include "hypha/action_log.h"
#include "hypha/archive_controller.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

namespace {

// Writes a minimal .tar.gz containing a single `hello.txt` entry with the given content.
// Exercises the real extraction path (Extract()) rather than faking a fixture on disk.
void WriteFixtureArchive(const std::string& path, const std::string& content) {
  struct archive* a = archive_write_new();
  archive_write_add_filter_gzip(a);
  archive_write_set_format_pax_restricted(a);
  ASSERT_EQ(archive_write_open_filename(a, path.c_str()), ARCHIVE_OK);

  struct archive_entry* entry = archive_entry_new();
  archive_entry_set_pathname(entry, "hello.txt");
  archive_entry_set_size(entry, static_cast<la_int64_t>(content.size()));
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  ASSERT_EQ(archive_write_header(a, entry), ARCHIVE_OK);
  ASSERT_EQ(archive_write_data(a, content.data(), content.size()), static_cast<la_ssize_t>(content.size()));
  archive_entry_free(entry);

  archive_write_close(a);
  archive_write_free(a);
}

Resource MakeDesiredResource(const std::string& source, const std::string& destination) {
  Resource res = {};
  res.info.name = strdup("test-archive");
  const std::string raw = "{\"source\": \"" + source + "\", \"destination\": \"" + destination + "\"}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class ArchiveControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewArchiveController();
    ASSERT_NE(ctrl, nullptr);
  }

  std::string TempPath(const char* suffix) const {
    return std::string(::testing::TempDir()) + "archive-controller-test-" + std::to_string(getpid()) + suffix;
  }
};

Controller* ArchiveControllerTest::ctrl = nullptr;

TEST_F(ArchiveControllerTest, ValidateFailsWithoutSourceOrDestination) {
  Resource res = {};
  res.info.name = strdup("bad-archive");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(ArchiveControllerTest, ValidateFailsWhenSourceDoesNotExist) {
  const std::string destination = TempPath("-missing-dest");
  Resource res = MakeDesiredResource("/nonexistent/archive.tar.gz", destination);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  ValidationLog vlog = {};
  InitValidationLog(&vlog, 4);
  EXPECT_FALSE(ControllerValidate(ctrl, &res, &vlog));
  EXPECT_GT(vlog.results_len, 0u);
  FreeValidationLog(&vlog, 4);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(ArchiveControllerTest, ObservePlanApplyExtractsArchiveAndBecomesIdempotent) {
  const std::string source = TempPath("-fixture.tar.gz");
  const std::string destination = TempPath("-dest");
  WriteFixtureArchive(source, "world");

  Resource res = MakeDesiredResource(source, destination);

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

  const std::string extracted = destination + "/hello.txt";
  FILE* f = fopen(extracted.c_str(), "r");
  ASSERT_NE(f, nullptr) << "expected " << extracted << " to exist after extraction";
  char buf[16] = {0};
  fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  EXPECT_STREQ(buf, "world");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  // Re-planning against an already-extracted destination should be a no-op.
  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
}

TEST_F(ArchiveControllerTest, StatusFailsWhenDestinationMissing) {
  const std::string source = TempPath("-fixture2.tar.gz");
  const std::string destination = TempPath("-never-extracted");
  WriteFixtureArchive(source, "world");

  Resource res = MakeDesiredResource(source, destination);
  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
}

TEST_F(ArchiveControllerTest, DiffFailsWhenDestinationMissing) {
  const std::string source = TempPath("-fixture3.tar.gz");
  const std::string destination = TempPath("-diff-never-extracted");
  WriteFixtureArchive(source, "world");

  Resource res = MakeDesiredResource(source, destination);
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

TEST_F(ArchiveControllerTest, DiffPassesWhenAllEntriesPresent) {
  const std::string source = TempPath("-fixture4.tar.gz");
  const std::string destination = TempPath("-diff-full-dest");
  WriteFixtureArchive(source, "world");

  Resource res = MakeDesiredResource(source, destination);
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
  EXPECT_NE(strstr(dlog.data[0].reason, "has all"), nullptr);
  FreeDeltaLog(&dlog);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
}

TEST_F(ArchiveControllerTest, DiffFailsWhenExtractedFileWasRemoved) {
  const std::string source = TempPath("-fixture5.tar.gz");
  const std::string destination = TempPath("-diff-partial-dest");
  WriteFixtureArchive(source, "world");

  Resource res = MakeDesiredResource(source, destination);
  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  // Remove the one file the fixture archive actually contains -- Diff should notice it's
  // gone even though Status would only check that the destination directory itself exists.
  remove((destination + "/hello.txt").c_str());

  DeltaLog dlog = {};
  InitDeltaLog(&dlog, 4);
  EXPECT_EQ(ControllerDiff(ctrl, &res, &dlog), kStatusInternalError);
  ASSERT_EQ(dlog.data_len, 2u) << "one delta for the missing entry, one summarizing the count";
  EXPECT_NE(strstr(dlog.data[0].reason, "hello.txt"), nullptr);
  EXPECT_NE(strstr(dlog.data[1].reason, "is missing"), nullptr);
  FreeDeltaLog(&dlog);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk) << "Status only checks the destination dir, not its contents";

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
}
