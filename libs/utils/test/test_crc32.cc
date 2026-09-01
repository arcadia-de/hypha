#include <gtest/gtest.h>

#include <cstring>

#include "hypha/crc32.h"

class Crc32Test : public ::testing::Test {};  // NOLINT(cppcoreguidelines-special-member-functions)

TEST_F(Crc32Test, Test_IsDeterministic) {
  const char* data = "hello, hypha";
  const uint32_t a = HyphaCrc32C(reinterpret_cast<const uint8_t*>(data), strlen(data));
  const uint32_t b = HyphaCrc32C(reinterpret_cast<const uint8_t*>(data), strlen(data));
  ASSERT_EQ(a, b);
}

TEST_F(Crc32Test, Test_DifferentInputsDiffer) {
  const char* a = "hello, hypha";
  const char* b = "hello, hyphb";
  ASSERT_NE(HyphaCrc32C(reinterpret_cast<const uint8_t*>(a), strlen(a)),
            HyphaCrc32C(reinterpret_cast<const uint8_t*>(b), strlen(b)));
}

TEST_F(Crc32Test, Test_EmptyInputIsZero) {
  // CRC-32C of a zero-length input is defined as 0 (init XOR final XOR both cancel out).
  ASSERT_EQ(HyphaCrc32C(reinterpret_cast<const uint8_t*>(""), 0), 0u);
}

TEST_F(Crc32Test, Test_MatchesKnownCrc32CVector) {
  // Standard CRC-32C check value for the ASCII string "123456789", per RFC 3720 (iSCSI) and
  // the common CRC-32C test vectors -- pins this down as CRC-32C (Castagnoli), not the plain
  // CRC-32/zlib polynomial the old `crc32` name collided with.
  const char* data = "123456789";
  ASSERT_EQ(HyphaCrc32C(reinterpret_cast<const uint8_t*>(data), strlen(data)), 0xE3069283u);
}
