#ifndef HYPHA_CONTROLLER_ACTION_H
#define HYPHA_CONTROLLER_ACTION_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_CONTROLLER_ACTION(V) \
  V(No)                               \
  V(Create)                           \
  V(Update)                           \
  V(Destroy)

// clang-format off
typedef enum {
#define DEFINE_ACTION(Name) k##Name##Action,
  FOR_EACH_CONTROLLER_ACTION(DEFINE_ACTION)
#undef DEFINE_ACTION
  kTotalNumberOfControllerActions,
} ControllerAction;
// clang-format on

static inline const char* ControllerActionToCString(const ControllerAction rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case k##Name##Action:       \
    return #Name;

    FOR_EACH_CONTROLLER_ACTION(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_CONTROLLER_ACTION_H
