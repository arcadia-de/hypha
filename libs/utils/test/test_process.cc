#include <gtest/gtest.h>

#include <cstring>

#include "hypha/process.h"

class ProcessTest : public ::testing::Test {};  // NOLINT(cppcoreguidelines-special-member-functions)

static int RunOnce(const char* bin, const char* arg) {
  Process p;
  memset(&p, 0, sizeof(Process));
  p.bin = bin;
  const char* args[1] = {arg};
  p.args = arg ? args : nullptr;
  p.num_args = arg ? 1 : 0;
  p.timeout = 5000;
  return ExecProcess(&p);
}

// Regression test: ExecProcess used to reap the child with waitpid(pid, &status, WNOHANG)
// right after both output pipes hit EOF. WNOHANG can return 0 (no state change available
// yet) in the narrow window before the kernel finishes tearing the child down, leaving
// `status` at its initialized value of 0 -- and WIFEXITED(0) is true with
// WEXITSTATUS(0) == 0, so a process that actually exited non-zero could be silently reported
// as a clean exit. This was intermittent (a race), not consistent, so it's exercised with
// many repeated runs rather than a single call.
TEST_F(ProcessTest, Test_ReportsNonZeroExitCodeReliably) {
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(RunOnce("/usr/bin/false", nullptr), 1) << "iteration " << i;
  }
}

TEST_F(ProcessTest, Test_ReportsZeroExitCodeReliably) {
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(RunOnce("/usr/bin/true", nullptr), 0) << "iteration " << i;
  }
}

TEST_F(ProcessTest, Test_ReportsSpecificNonZeroExitCode) {
  // `sh -c "exit 42"` -- a specific, non-1 exit code, to rule out a fix that merely
  // special-cases "0 vs 1" without actually reading the real exit status.
  Process p;
  memset(&p, 0, sizeof(Process));
  p.bin = "/bin/sh";
  const char* args[2] = {"-c", "exit 42"};
  p.args = args;
  p.num_args = 2;
  p.timeout = 5000;
  EXPECT_EQ(ExecProcess(&p), 42);
}
