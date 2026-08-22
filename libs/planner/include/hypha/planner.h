#ifndef HYPHA_PLANNER_H
#define HYPHA_PLANNER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdarg.h>
#include <stdio.h>

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

#define DEFINE_NEW_PLANNED_ACTION(Name)                                                                  \
  static inline PlannedAction* New##Name##PlannedAction(Plan* pl, Resource* res, const char* fmt, ...) { \
    ASSERT(pl);                                                                                          \
    ASSERT(res);                                                                                         \
    ASSERT(fmt);                                                                                         \
    PlannedAction* action = NewPlannedAction(pl);                                                        \
    if (action) {                                                                                        \
      action->action = k##Name##Action;                                                                  \
      action->resource = res;                                                                            \
      clock_gettime(CLOCK_REALTIME, &action->timestamp);                                                 \
      va_list args;                                                                                      \
      va_start(args, fmt);                                                                               \
      vsnprintf(action->reason, HYPHA_REASON_MAX_LENGTH, fmt, args);                                     \
      va_end(args);                                                                                      \
    }                                                                                                    \
    return action;                                                                                       \
  }
FOR_EACH_CONTROLLER_ACTION(DEFINE_NEW_PLANNED_ACTION)
#undef DEFINE_NEW_PLANNED_ACTION

static inline bool IsPlanEmpty(const Plan* rhs) {
  return !rhs || rhs->actions_len == 0;
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_PLANNER_H
