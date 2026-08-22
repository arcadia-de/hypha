#include "hypha/validation_result.h"

#include "hypha/resource.h"

int DefaultValidationResultComparator(const void* lhs, const void* rhs) {
  const ValidationResult* a = (const ValidationResult*)lhs;
  const ValidationResult* b = (const ValidationResult*)rhs;

  const int idc = uuid_compare(a->resource->id, b->resource->id);
  if (idc != 0)
    return idc;

  // TODO(@s0cks): update sorting
  return 0;
}
