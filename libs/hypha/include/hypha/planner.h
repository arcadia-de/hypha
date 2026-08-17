#ifndef HYPHA_PLANNER_H
#define HYPHA_PLANNER_H

#include "hypha.h"

typedef struct {
  char* id;
  ControllerAction action;
  Reason reason;
} PlannedAction;

typedef struct {
  PlannedAction* actions;
  size_t actions_len;
  size_t actions_cap;
} Plan;

void InitPlan(Plan* pl, const size_t init_cap);
typedef bool (*VisitPlannedActionFn)(size_t idx, const PlannedAction* action, void* data);
void VisitPlannedActions(const Plan* pl, VisitPlannedActionFn fn, void* data);
void AppendPlannedAction(Plan* pl, PlannedAction* rhs);
void FreePlan(Plan* pl);

static inline bool IsPlanEmpty(const Plan* rhs) {
  return !rhs || rhs->actions_len == 0;
}

#endif  // HYPHA_PLANNER_H
