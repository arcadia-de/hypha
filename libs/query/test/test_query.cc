#include <gtest/gtest.h>

#include "hypha/query.h"

class QueryTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  QueryTest() = default;
  ~QueryTest() override = default;
};

TEST_F(QueryTest, Test) {
  ASSERT_TRUE(true);
}
