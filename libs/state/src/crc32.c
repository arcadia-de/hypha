#include "crc32.h"

#include <stdbool.h>

static uint32_t sw_table[256];
static bool sw_table_ready = false;

static inline void EnsureSoftwareTable(void) {
  if (sw_table_ready)
    return;

  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i;
    for (int k = 0; k < 8; k++)
      c = (c & 1) ? (0x82F63B78u ^ (c >> 1)) : (c >> 1);  // reversed CRC-32C poly
    sw_table[i] = c;
  }
  sw_table_ready = true;
}

static inline uint32_t Crc32cSoftware(const uint8_t* data, size_t len) {
  EnsureSoftwareTable();

  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++)
    crc = sw_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);

  return crc ^ 0xFFFFFFFFu;
}

#if defined(__x86_64__) || defined(__i386__)
#include <nmmintrin.h>

__attribute__((target("sse4.2"))) static inline uint32_t Crc32cHardwareX86(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;

  while (len >= 8) {
    crc = (uint32_t)_mm_crc32_u64(crc, *(const uint64_t*)data);
    data += 8;
    len -= 8;
  }

  while (len > 0) {
    crc = _mm_crc32_u8(crc, *data);
    data += 1;
    len -= 1;
  }

  return crc ^ 0xFFFFFFFFu;
}

#define HAVE_X86_CRC32C 1
#endif

#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
#include <arm_acle.h>

static inline uint32_t Crc32cHardwareArm(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;

  while (len >= 8) {
    crc = __crc32cd(crc, *(const uint64_t*)data);
    data += 8;
    len -= 8;
  }
  while (len > 0) {
    crc = __crc32cb(crc, *data);
    data += 1;
    len -= 1;
  }

  return crc ^ 0xFFFFFFFFu;
}

#define HAVE_ARM_CRC32C 1
#endif

static inline bool IsHardwareAccelerationSupported() {
  static int supported = -1;
  if (supported == -1)
    supported = __builtin_cpu_supports("sse4.2") ? 1 : 0;
  return supported;
}

uint32_t crc32(const uint8_t* bytes, size_t nbytes) {
#ifdef HYPHA_CRC32_FORCE_SOFTWARE_FOR_TESTING
  return Crc32cSoftware(bytes, nbytes);
#endif

#if defined(HAVE_X86_CRC32C)
  if (IsHardwareAccelerationSupported())
    return Crc32cHardwareX86(bytes, nbytes);

  return Crc32cSoftware(bytes, nbytes);

#elif defined(HAVE_ARM_CRC32C)
  return Crc32cHardwareArm(bytes, nbytes);

#else
  return Crc32cSoftware(bytes, nbytes);

#endif
}
