#ifndef HYPHA_VALIDATION_LOG_H
#define HYPHA_VALIDATION_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/validation_result.h"

typedef struct {
  ValidationResult* results;
  size_t results_len;
  size_t results_cap;
} ValidationLog;

typedef bool (*VisitValidationLogFn)(const size_t idx, ValidationResult* value, void* data);

void InitValidationLog(ValidationLog* vl, const size_t init_cap);
ValidationResult* NewValidationResult(ValidationLog* vl);
void AppendValidationResult(ValidationLog* vl, ValidationResult* rhs);
void AppendValidationLog(ValidationLog* vl, ValidationLog* rhs);
void VisitAllValidationResults(ValidationLog* vl, VisitValidationLogFn fn, void* data);
void FreeValidationLog(ValidationLog* vl, const size_t init_cap);

#define DEFINE_NEW_VALIDATION_RESULT(Kind)                                                                         \
  static inline ValidationResult* New##Kind##ValidationResult(ValidationLog* vlog, Resource* res, const char* fmt, \
                                                              ...) {                                               \
    ValidationResult* new_result = NewValidationResult(vlog);                                                      \
    if (new_result) {                                                                                              \
      new_result->kind = kValidation##Kind;                                                                        \
      new_result->resource = res;                                                                                  \
      if (fmt) {                                                                                                   \
        va_list args;                                                                                              \
        va_start(args, fmt);                                                                                       \
        snprintf(new_result->reason, HYPHA_REASON_MAX_LENGTH, fmt, args);                                          \
        va_end(args);                                                                                              \
      }                                                                                                            \
    }                                                                                                              \
    return new_result;                                                                                             \
  }

FOR_EACH_VALIDATION_RESULT_KIND(DEFINE_NEW_VALIDATION_RESULT);
#undef DEFINE_NEW_VALIDATION_RESULT

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_VALIDATION_LOG_H
