#ifndef HYPHA_RESOURCE_STATE_H
#define HYPHA_RESOURCE_STATE_H

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

static inline const char* ResourceStateName(const ResourceState rhs) {
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

#endif  // HYPHA_RESOURCE_STATE_H
