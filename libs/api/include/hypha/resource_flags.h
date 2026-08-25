#ifndef HYPHA_RESOURCE_FLAGS_H
#define HYPHA_RESOURCE_FLAGS_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>
#include <stdint.h>

// Per-resource behavioral flags, packed into a single byte.
//
// Bit position is pinned explicitly per-flag (rather than left to enum ordering) because
// a ResourceFlags value travels through the same channels as any other resource field --
// state store, history log, query engine, eventual wire formats -- so once anything
// persists or serializes it, the bit layout is effectively on-disk/wire format, not an
// implementation detail. Adding a flag means picking the next free bit and appending a
// line to FOR_EACH_RESOURCE_FLAG below; it must never mean renumbering an existing one.
//
// Bit 0 (Static) is the only flag defined today: it marks a resource as compiled-in and
// pre-converged (see IsResourceStatic in resource.h). Bits 1-7 are reserved for future use
// -- e.g. a Provider bit distinguishing provider-style resources (Controller,
// PackageBackend, ...) from the things they provide for, without a string comparison on
// kind, or a Protected bit rejecting manifest-driven destroy/mutation of a given resource.
// This is exactly why a bitfield was chosen over a plain enum-valued "mode" field: multiple
// independent axes can be added later without a schema migration on the field itself.
#define FOR_EACH_RESOURCE_FLAG(V) \
  V(Static, 0) /* compiled-in, pre-converged; bypasses observe/normalize/plan/apply entirely */

typedef uint8_t ResourceFlags;

// clang-format off
typedef enum {
#define DEFINE_BIT(Name, Bit) kResourceFlagBit##Name = (Bit),
  FOR_EACH_RESOURCE_FLAG(DEFINE_BIT)
#undef DEFINE_BIT
} ResourceFlagBit;
// clang-format on

#define DEFINE_FLAG_MASK(Name, Bit) static const ResourceFlags kResourceFlag##Name = (ResourceFlags)(1u << (Bit));
FOR_EACH_RESOURCE_FLAG(DEFINE_FLAG_MASK)
#undef DEFINE_FLAG_MASK

static const ResourceFlags kResourceFlagsNone = 0;

static inline bool ResourceFlagsHas(const ResourceFlags flags, const ResourceFlags mask) {
  return (flags & mask) == mask;
}

static inline ResourceFlags ResourceFlagsSet(const ResourceFlags flags, const ResourceFlags mask) {
  return (ResourceFlags)(flags | mask);
}

static inline ResourceFlags ResourceFlagsClear(const ResourceFlags flags, const ResourceFlags mask) {
  return (ResourceFlags)(flags & (ResourceFlags)~mask);
}

static inline const char* ResourceFlagBitName(const ResourceFlagBit bit) {
  switch (bit) {
#define DEFINE_CASE(Name, Bit) \
  case kResourceFlagBit##Name: \
    return #Name;
    FOR_EACH_RESOURCE_FLAG(DEFINE_CASE)
#undef DEFINE_CASE
    default:
      return "Unknown";
  }
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RESOURCE_FLAGS_H
