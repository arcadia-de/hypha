#include <gtest/gtest.h>

#include "hypha/ignore.h"

class IgnoreTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  IgnoreTest() = default;
  ~IgnoreTest() override = default;
};

TEST_F(IgnoreTest, Test_New_Null) {
  Ignore* ig = NewIgnore(NULL);
  ASSERT_NE(ig, nullptr);
  ASSERT_TRUE(IgnoreIsEmpty(ig));
}

TEST_F(IgnoreTest, Test_Matches_Empty) {
  Ignore* ig = NewIgnore(NULL);
  ASSERT_FALSE(IgnoreMatches(ig, "/test.txt"));
}

TEST_F(IgnoreTest, Test_Matches) {
  Ignore* ig = NewIgnore(NULL);
  ASSERT_TRUE(IgnoreIsEmpty(ig));
  ASSERT_FALSE(IgnoreMatches(ig, "/test.txt"));
}

TEST_F(IgnoreTest, Test_Append_Pattern) {
  Ignore* ig = NewIgnore(NULL);
  ASSERT_TRUE(IgnoreIsEmpty(ig));
  ASSERT_FALSE(IgnoreMatches(ig, "/test.txt"));

  AppendIgnorePattern(ig, "*.txt");
  ASSERT_FALSE(IgnoreIsEmpty(ig));
  ASSERT_TRUE(IgnoreMatches(ig, "/test.txt"));
}
