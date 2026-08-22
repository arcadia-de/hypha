#ifndef HYPHA_VALIDATION_RESULT_H
#define HYPHA_VALIDATION_RESULT_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include "hypha.h"

#define FOR_EACH_VALIDATION_RESULT_KIND(V) \
  V(Skipped)                               \
  V(Passed)                                \
  V(Warning)                               \
  V(Failed)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) kValidation##Name,
  FOR_EACH_VALIDATION_RESULT_KIND(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfValidationResultKinds,
} ValidationResultKind;
// clang-format on

typedef struct {
  ValidationResultKind kind;
  Resource* resource;
  Reason reason;
} ValidationResult;

typedef int (*ValidationResultComparator)(const void*, const void*);

int DefaultValidationResultComparator(const void*, const void*);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_VALIDATION_RESULT_H
