#ifndef HYPHA_VALIDATION_LOG_H
#define HYPHA_VALIDATION_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

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

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_VALIDATION_LOG_H
