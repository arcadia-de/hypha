#ifndef HYPHA_PLANNED_ACTION_H
#define HYPHA_PLANNED_ACTION_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <time.h>
#include <uuid/uuid.h>

#include "hypha.h"
#include "hypha/reason.h"

typedef struct {
  struct timespec timestamp;
  ControllerAction action;
  Reason reason;
  Resource* resource;
} PlannedAction;

typedef int (*PlannedActionComparator)(const void* lhs, const void* rhs);

int DefaultPlannedActionComparator(const void* lhs, const void* rhs);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_PLANNED_ACTION_H
