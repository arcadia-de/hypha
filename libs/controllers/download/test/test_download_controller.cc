#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <sodium.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha/action_log.h"
#include "hypha/download_controller.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

namespace {

std::string FileUrl(const std::string& path) {
  return "file://" + path;
}

std::string Sha256Hex(const std::string& content) {
  unsigned char digest[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(digest, reinterpret_cast<const unsigned char*>(content.data()), content.size());
  static const char kHex[] = "0123456789abcdef";
  std::string out(crypto_hash_sha256_BYTES * 2, '\0');
  for (size_t i = 0; i < sizeof(digest); i++) {
    out[i * 2] = kHex[digest[i] >> 4];
    out[i * 2 + 1] = kHex[digest[i] & 0x0F];
  }
  return out;
}

Resource MakeDesiredResource(const std::string& url, const std::string& destination, const char* sha256 = nullptr) {
  Resource res = {};
  res.info.name = strdup("test-download");
  std::string raw = "{\"url\": \"" + url + "\", \"destination\": \"" + destination + "\"";
  if (sha256)
    raw += ", \"sha256\": \"" + std::string(sha256) + "\"";
  raw += "}";
  res.spec.raw = strdup(raw.c_str());
  EXPECT_TRUE(ResourceSpecParseJson(&res.spec));
  return res;
}

}  // namespace

class DownloadControllerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  static Controller* ctrl;

  static void SetUpTestSuite() {
    ctrl = NewDownloadController();
    ASSERT_NE(ctrl, nullptr);
    ControllerInit(ctrl);
  }

  static void TearDownTestSuite() {
    ControllerDeInit(ctrl);
  }

  std::string TempPath(const char* suffix) const {
    return std::string(::testing::TempDir()) + "download-controller-test-" + std::to_string(getpid()) + suffix;
  }
};

Controller* DownloadControllerTest::ctrl = nullptr;

TEST_F(DownloadControllerTest, ValidateFailsWithoutUrlOrDestination) {
  Resource res = {};
  res.info.name = strdup("bad-download");
  res.spec.raw = strdup("{}");
  ASSERT_TRUE(ResourceSpecParseJson(&res.spec));

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}

TEST_F(DownloadControllerTest, ValidateFailsWithMalformedSha256) {
  const std::string destination = TempPath("-dest1");
  Resource res = MakeDesiredResource(FileUrl("/etc/hosts"), destination, "not-a-real-digest");

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

TEST_F(DownloadControllerTest, ObservePlanApplyDownloadsFileAndBecomesIdempotent) {
  const std::string source = TempPath("-source.txt");
  FILE* sf = fopen(source.c_str(), "w");
  ASSERT_NE(sf, nullptr);
  fputs("hello from download test", sf);
  fclose(sf);

  const std::string destination = TempPath("-dest2");
  Resource res = MakeDesiredResource(FileUrl(source), destination);

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

  FILE* f = fopen(destination.c_str(), "r");
  ASSERT_NE(f, nullptr);
  char buf[64] = {0};
  fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  EXPECT_STREQ(buf, "hello from download test");

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  Plan replan = {};
  InitPlan(&replan, 4);
  EXPECT_EQ(ControllerPlan(ctrl, &res, &res, &replan), kNoAction);
  FreePlan(&replan);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
  remove(destination.c_str());
}

TEST_F(DownloadControllerTest, ChecksumMismatchTriggersUpdateAndGetsFixed) {
  const std::string content = "expected content";
  const std::string source = TempPath("-source2.txt");
  FILE* sf = fopen(source.c_str(), "w");
  ASSERT_NE(sf, nullptr);
  fputs(content.c_str(), sf);
  fclose(sf);

  const std::string destination = TempPath("-dest3");
  FILE* df = fopen(destination.c_str(), "w");
  ASSERT_NE(df, nullptr);
  fputs("stale content", df);
  fclose(df);

  const std::string digest = Sha256Hex(content);
  Resource res = MakeDesiredResource(FileUrl(source), destination, digest.c_str());

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  EXPECT_EQ(action, kUpdateAction);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  ASSERT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusOk);

  FILE* f = fopen(destination.c_str(), "r");
  ASSERT_NE(f, nullptr);
  char buf[64] = {0};
  fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  EXPECT_STREQ(buf, content.c_str());

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusOk);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
  remove(destination.c_str());
}

TEST_F(DownloadControllerTest, ApplyFailsAndLeavesNoTempFileOnChecksumMismatch) {
  const std::string source = TempPath("-source3.txt");
  FILE* sf = fopen(source.c_str(), "w");
  ASSERT_NE(sf, nullptr);
  fputs("actual content", sf);
  fclose(sf);

  const std::string destination = TempPath("-dest4");
  // 64 valid hex chars, but not the real digest of `source`'s contents.
  const std::string wrong_digest(64, 'a');
  Resource res = MakeDesiredResource(FileUrl(source), destination, wrong_digest.c_str());

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  Plan plan = {};
  InitPlan(&plan, 4);
  ControllerAction action = ControllerPlan(ctrl, &res, &res, &plan);
  EXPECT_EQ(action, kCreateAction);
  FreePlan(&plan);

  AppliedActionLog alog = {};
  EXPECT_EQ(ControllerApply(ctrl, &res, action, &alog), kStatusInternalError);

  struct stat st;
  EXPECT_NE(stat(destination.c_str(), &st), 0) << "destination should not have been created";

  const std::string tmp_path = destination + ".hypha-download-tmp." + std::to_string(getpid());
  EXPECT_NE(stat(tmp_path.c_str(), &st), 0) << "temp file should have been cleaned up";

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
  remove(source.c_str());
}

TEST_F(DownloadControllerTest, StatusFailsWhenDestinationMissing) {
  const std::string destination = TempPath("-never-downloaded");
  Resource res = MakeDesiredResource(FileUrl("/etc/hosts"), destination);

  StateEntry last = {};
  ASSERT_EQ(ControllerObserve(ctrl, &res, &last), kStatusOk);

  EXPECT_EQ(ControllerStat(ctrl, &res), kStatusInternalError);

  free(res.info.name);
  FreeResourceSpecJson(&res.spec);
  free(res.spec.raw);
}
