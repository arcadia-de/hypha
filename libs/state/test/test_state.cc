#include <cstdint>
#include <gtest/gtest.h>

#include "hypha/state.h"

class StateEntryTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  StateEntryTest() = default;
  ~StateEntryTest() override = default;
};

TEST_F(StateEntryTest, Test_EncodeDecode) {
  static const char* json = "{}";
  static const char* kind = "Controller";
  static const char* id = "12861612";
  static const bool orphaned = false;
  static const uint64_t hash = 12789126;
  static const uint32_t last_status = 12;
  static const time_t applied_at = 12897129;

  const StateEntry in = {
      .orphaned = orphaned,
      .id = const_cast<char*>(id),      // NOLINT(cppcoreguidelines-pro-type-const-cast)
      .kind = const_cast<char*>(kind),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
      .hash = hash,
      .observed_json = const_cast<char*>(json),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
      .last_status = last_status,
      .applied_at = applied_at,
  };
  const size_t expected_len = 1 + (sizeof(size_t) + strlen(kind)) + sizeof(uint64_t) + (sizeof(size_t) + strlen(json)) +
                              sizeof(uint32_t) + sizeof(time_t);
  uint8_t* value = NULL;
  size_t value_len = 0;
  EncodeStateEntry(&in, &value, &value_len);
  ASSERT_NE(value, nullptr);
  ASSERT_EQ(value_len, expected_len);

  StateEntry out;
  DecodeStateEntry(value, &out);
  ASSERT_EQ(in.orphaned, out.orphaned);
  ASSERT_STREQ(in.kind, out.kind);
  ASSERT_EQ(in.hash, out.hash);
  ASSERT_STREQ(in.observed_json, out.observed_json);
  ASSERT_EQ(in.last_status, out.last_status);
  ASSERT_EQ(in.applied_at, out.applied_at);
}
