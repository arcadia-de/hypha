#include <gtest/gtest.h>

#include "hypha/resource.h"

class ResourceTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  ResourceTest() = default;
  ~ResourceTest() override = default;
};

TEST_F(ResourceTest, Test) {
  ASSERT_TRUE(true);
}
