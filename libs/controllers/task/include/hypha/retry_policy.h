#ifndef HYPHA_RETRY_POLICY_H
#define HYPHA_RETRY_POLICY_H

#include <strings.h>

#include "hypha/assertions.h"

#define FOR_EACH_RETRY_POLICY(V) \
  V(Always)                      \
  V(Never)

// clang-format off
typedef enum {
#define DEFINE_POLICY(Name) \
  kRetryPolicy##Name,

  FOR_EACH_RETRY_POLICY(DEFINE_POLICY)
#undef DEFINE_POLICY
  kTotalNumberOfRetryPolicies,
  kDefaultRetryPolicy = kRetryPolicyNever,
} RetryPolicy;
// clang-format on

static inline RetryPolicy ParseRetryPolicy(const char* rhs) {
  if (!rhs)
    goto default_policy;

  // clang-format off
#define DEFINE_CHECK(Name) \
  else if(strcasecmp(#Name, rhs) == 0) \
    return kRetryPolicy##Name;

  FOR_EACH_RETRY_POLICY(DEFINE_CHECK)
#undef DEFINE_CHECK
  // clang-format on

default_policy:
  return kDefaultRetryPolicy;
}

static inline const char* RetryPolicyToCStr(const RetryPolicy rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case kRetryPolicy##Name:    \
    return #Name;

    FOR_EACH_RETRY_POLICY(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#endif  // HYPHA_RETRY_POLICY_H
