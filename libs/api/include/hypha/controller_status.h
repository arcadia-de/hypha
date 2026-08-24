#ifndef HYPHA_CONTROLLER_STATUS_H
#define HYPHA_CONTROLLER_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_CONTROLLER_STATUS(V) \
  V(Ok)                               \
  V(NoOp)                             \
  V(InvalidSpec)                      \
  V(NotFound)                         \
  V(Conflict)                         \
  V(Unsupported)                      \
  V(TransientError)                   \
  V(PermanentError)                   \
  V(InternalError)

// clang-format off
typedef enum {
#define DEFINE_STATUS(Name) kStatus##Name,
  FOR_EACH_CONTROLLER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
  kTotalNumberOfControllerStatuses,
} ControllerStatus;
// clang-format on

static inline const char* ControllerStatusToCString(const ControllerStatus rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case kStatus##Name:         \
    return #Name;

    FOR_EACH_CONTROLLER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_CONTROLLER_STATUS_H
