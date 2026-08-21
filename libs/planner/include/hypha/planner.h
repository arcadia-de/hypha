#ifndef HYPHA_PLANNER_H
#define HYPHA_PLANNER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include "hypha.h"
#include "hypha/planned_action.h"

typedef struct {
  PlannedAction* actions;
  size_t actions_len;
  size_t actions_cap;
} Plan;

typedef bool (*VisitPlannedActionFn)(size_t idx, const PlannedAction* action, void* data);

void InitPlan(Plan* pl, const size_t init_cap);
void VisitPlannedActions(const Plan* pl, VisitPlannedActionFn fn, void* data);
PlannedAction* NewPlannedAction(Plan* pl);
void AppendPlannedAction(Plan* pl, PlannedAction* rhs);
void AppendPlan(Plan* pl, const Plan* rhs);
void FreePlan(Plan* pl);

static inline bool IsPlanEmpty(const Plan* rhs) {
  return !rhs || rhs->actions_len == 0;
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_PLANNER_H
