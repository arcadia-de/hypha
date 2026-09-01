#ifndef HYPHA_RESOURCE_STATE_H
#define HYPHA_RESOURCE_STATE_H

#include <strings.h>

#define FOR_EACH_RESOURCE_STATE(V) \
  V(Pending)                       \
  V(Processing)                    \
  V(Ready)                         \
  V(Failed)                        \
  V(Unknown)

// clang-format off
typedef enum {
#define DEFINE_STATE(Name) kResource##Name,
  FOR_EACH_RESOURCE_STATE(DEFINE_STATE)
#undef DEFINE_STATE
  kTotalNumberOfResourceStates,
} ResourceState;
// clang-format on

static inline const char* ResourceStateCStr(const ResourceState rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case kResource##Name:       \
    return #Name;
    FOR_EACH_RESOURCE_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

static inline ResourceState ParseResourceState(const char* value) {
  if (!value)
    return kResourceUnknown;

  // clang-format off
#define DEFINE_CHECK(Name) \
  else if(strcasecmp(#Name, value) == 0) \
    return kResource##Name;
  FOR_EACH_RESOURCE_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  // clang-format on

  return kResourceUnknown;
}

#endif  // HYPHA_RESOURCE_STATE_H
