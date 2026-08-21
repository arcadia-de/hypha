#ifndef HYPHA_PLANNED_ACTION_H
#define HYPHA_PLANNED_ACTION_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <time.h>

#include "hypha.h"

typedef struct {
  char* id;
  struct timespec timestamp;
  ControllerAction action;
  Reason reason;
} PlannedAction;

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_PLANNED_ACTION_H
