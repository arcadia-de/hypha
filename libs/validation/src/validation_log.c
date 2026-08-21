#include "hypha/validation_log.h"

#include <string.h>

#include "hypha/log.h"
#include "hypha/validation_result.h"

void InitValidationLog(ValidationLog* vl, const size_t init_cap) {
  ASSERT(vl);
  if (init_cap > 0) {
    const size_t total_size = sizeof(ValidationResult) * init_cap;
    ValidationResult* new_results = (ValidationResult*)malloc(total_size);
    LOG_FATAL_IF(!new_results, "failed to allocate ValidationLog of %zu", init_cap);
    memset(new_results, 0, total_size);
    vl->results = new_results;
    vl->results_len = 0;
    vl->results_cap = init_cap;
  }
}

static inline void EnsureLength(ValidationLog* vl, const size_t new_len) {
  ASSERT(vl);
  if (new_len >= vl->results_cap) {
    const size_t new_cap = (vl->results_cap + new_len) * 2;
    const size_t total_size = sizeof(ValidationResult) * new_cap;
    ValidationResult* new_results = (ValidationResult*)malloc(total_size);
    LOG_FATAL_IF(!new_results, "failed to allocate new ValidationLog of size %zu", new_cap);
    vl->results = new_results;
    vl->results_cap = new_cap;
  }
}

ValidationResult* NewValidationResult(ValidationLog* vl) {
  ASSERT(vl);
  EnsureLength(vl, vl->results_len + 1);
  ASSERT(vl->results_cap > (vl->results_len + 1));
  ValidationResult* next = &vl->results[vl->results_len];
  vl->results_len++;
  memset(next, 0, sizeof(ValidationResult));
  return next;
}

void AppendValidationResult(ValidationLog* vl, ValidationResult* rhs) {
  ASSERT(vl);
  EnsureLength(vl, vl->results_len + 1);
  ASSERT(vl->results_cap > (vl->results_len + 1));
  memcpy(&vl->results[vl->results_len], rhs, sizeof(ValidationResult));
  vl->results_len++;
}

void AppendValidationLog(ValidationLog* vl, ValidationLog* rhs) {
  ASSERT(vl);
  ASSERT(rhs);
  ASSERT(rhs->results);
  EnsureLength(vl, vl->results_len + rhs->results_len);
  ASSERT(vl->results_cap > (vl->results_len + rhs->results_len));
  memcpy(&vl->results[vl->results_len], &rhs->results[0], sizeof(ValidationResult) * rhs->results_len);
  vl->results_len += rhs->results_len;
}

void VisitAllValidationResults(ValidationLog* vl, VisitValidationLogFn fn, void* data) {
  ASSERT(vl);
  ASSERT(fn);
  for (size_t i = 0; i < vl->results_len; i++) {
    if (!fn(i, &vl->results[i], data))
      return;
  }
}

void FreeValidationLog(ValidationLog* vl, const size_t init_cap) {
  ASSERT(vl);
  if (vl->results)
    free(vl->results);

  if (init_cap > 0)
    InitValidationLog(vl, init_cap);
}
