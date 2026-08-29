#ifndef HYPHA_STRUCTURED_LOG_H
#define HYPHA_STRUCTURED_LOG_H

#include <stdlib.h>

typedef struct _StructuredLog StructuredLog;

void InitValidationLog(ValidationLog* vl, const size_t init_cap);
ValidationResult* NewValidationResult(ValidationLog* vl);
void AppendValidationResult(ValidationLog* vl, ValidationResult* rhs);
void AppendValidationLog(ValidationLog* vl, ValidationLog* rhs);
void VisitAllValidationResults(ValidationLog* vl, VisitValidationLogFn fn, void* data);
void FreeValidationLog(ValidationLog* vl, const size_t init_cap);
void SortValidationLog(ValidationLog* vl, ValidationResultComparator compare);

#endif  // HYPHA_STRUCTURED_LOG_H
