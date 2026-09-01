#ifndef HYPHA_CRC32_H
#define HYPHA_CRC32_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// NOTE: this is CRC-32C (Castagnoli, polynomial 0x1EDC6F41 / reversed 0x82F63B78), not the
// classic CRC-32 (polynomial 0x04C11DB7) used by zlib/gzip -- the two are not
// interchangeable and produce different digests for the same input. The function used to be
// named plain `crc32`, which is also zlib's exported symbol name; any binary linking both
// this translation unit and a zlib-linked library (e.g. libarchive, for gzip support) would
// have the dynamic linker resolve the *other* library's internal calls to zlib's crc32() to
// this one instead, silently corrupting their checksums. `HyphaCrc32C` avoids the collision
// outright and documents which variant this actually is.
uint32_t HyphaCrc32C(const uint8_t* bytes, size_t nbytes);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // HYPHA_CRC32_H
