#ifndef HYPHA_TASK_POLICY_H
#define HYPHA_TASK_POLICY_H

#include <strings.h>

#include "hypha/assertions.h"

#define FOR_EACH_TASK_POLICY(V) \
  V(Always)                     \
  V(OnChange)                   \
  V(Once)

// clang-format off
typedef enum {
#define DEFINE_POLICY(Name) \
  kTaskPolicy##Name,

  FOR_EACH_TASK_POLICY(DEFINE_POLICY)
#undef DEFINE_POLICY

  kTotalNumberOfTaskPolicies,
  kDefaultTaskPolicy = kTaskPolicyOnChange,
} TaskPolicy;
// clang-format on

static inline TaskPolicy ParseTaskPolicy(const char* rhs) {
  if (!rhs)
    goto default_policy;

  // clang-format off
#define DEFINE_CHECK(Name) \
  else if(strcasecmp(#Name, rhs) == 0) \
    return kTaskPolicy##Name;

  FOR_EACH_TASK_POLICY(DEFINE_CHECK)
#undef DEFINE_CHECK
  // clang-format on

default_policy:
  return kDefaultTaskPolicy;
}

static inline const char* TaskPolicyToCStr(const TaskPolicy rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case kTaskPolicy##Name:     \
    return #Name;

    FOR_EACH_TASK_POLICY(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#endif  // HYPHA_TASK_POLICY_H
