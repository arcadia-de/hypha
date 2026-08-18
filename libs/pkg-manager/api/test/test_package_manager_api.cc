#include <gtest/gtest.h>

class PackageManagerTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  PackageManagerTest() = default;
  ~PackageManagerTest() override = default;
};

TEST_F(PackageManagerTest, Test) {
  ASSERT_TRUE(true);
}
