#include "hypha/planned_action.h"

#include "hypha/resource.h"

int DefaultPlannedActionComparator(const void* lhs, const void* rhs) {
  const PlannedAction* a = (const PlannedAction*)lhs;
  const PlannedAction* b = (const PlannedAction*)rhs;

  const int idc = uuid_compare(a->resource->id, b->resource->id);
  if (idc != 0)
    return idc;

  // TODO(@s0cks): compare timestamp
  return 0;
}
