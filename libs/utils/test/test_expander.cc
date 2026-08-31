#include <gtest/gtest.h>

#include "hypha/expander.h"

class ExpanderTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 protected:
  Expander expander_{};

  inline auto expander() -> Expander* {
    return &expander_;
  }

 public:
  ExpanderTest() = default;
  ~ExpanderTest() override = default;
};

TEST_F(ExpanderTest, Test_ExpandTilda) {
  const char* pattern = "~/test";

  char* result = NULL;
  size_t result_len = 0;
  ASSERT_TRUE(ExpandStr(expander(), pattern, &result, &result_len));
  printf("result: %s\n", result);
}

TEST_F(ExpanderTest, Test_Var) {
  const char* pattern = "$LUA_PATH";

  char* result = NULL;
  size_t result_len = 0;
  ASSERT_TRUE(ExpandStr(expander(), pattern, &result, &result_len));
  printf("result: %s\n", result);
}

TEST_F(ExpanderTest, Test_VarDefault) {
  const char* pattern = "${CUSTOM_VALUE:10}";

  char* result = NULL;
  size_t result_len = 0;
  ASSERT_TRUE(ExpandStr(expander(), pattern, &result, &result_len));
  printf("result: %s\n", result);
}

TEST_F(ExpanderTest, Test_VarDefaultHyphen) {
  const char* pattern = "${CUSTOM_VALUE:-10}";

  char* result = NULL;
  size_t result_len = 0;
  ASSERT_TRUE(ExpandStr(expander(), pattern, &result, &result_len));
  printf("result: %s\n", result);
}

static inline auto ResolveCustomSymbol(const char sym, void* data) -> const char* {
  switch (sym) {
    case 't':
      return "Time";
    default:
      return NULL;
  }
}

TEST_F(ExpanderTest, Test_CustomSymbolNotFound) {
  const char* pattern = "%f";

  Expander expander;
  expander.resolve = &ResolveCustomSymbol;

  char* result = NULL;
  size_t result_len = 0;
  ASSERT_TRUE(ExpandStr(&expander, pattern, &result, &result_len));
  printf("result: %s\n", result);
}

TEST_F(ExpanderTest, Test_CustomSymbol) {
  const char* pattern = "%t";

  Expander expander;
  expander.resolve = &ResolveCustomSymbol;

  char* result = NULL;
  size_t result_len = 0;
  ASSERT_TRUE(ExpandStr(&expander, pattern, &result, &result_len));
  printf("result: %s\n", result);
}
