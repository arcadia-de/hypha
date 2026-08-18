#include <gtest/gtest.h>

class ControllerApiTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  ControllerApiTest() = default;
  ~ControllerApiTest() override = default;
};

TEST_F(ControllerApiTest, Test) {
  ASSERT_TRUE(true);
}
