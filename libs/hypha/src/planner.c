#include "hypha/planner.h"

#include <stdint.h>
#include <string.h>

void InitPlan(Plan* pl, const size_t init_cap) {
  if (!pl || init_cap == 0)
    return;

  const size_t total_size = sizeof(PlannedAction) * init_cap;
  PlannedAction* actions = (PlannedAction*)malloc(total_size);
  if (!actions)
    return;

  memset(actions, 0, total_size);
  pl->actions = actions;
  pl->actions_len = 0;
  pl->actions_cap = init_cap;
}

void VisitPlannedActions(const Plan* pl, VisitPlannedActionFn fn, void* data) {
  if (IsPlanEmpty(pl))
    return;

  for (size_t i = 0; i < pl->actions_len; i++) {
    if (!fn(i, &pl->actions[i], data))
      return;
  }
}

void AppendPlannedAction(Plan* pl, PlannedAction* action) {
  if (!pl || !action)
    return;

  if ((pl->actions_len + 1) >= pl->actions_cap) {
    const size_t new_cap = pl->actions_cap * 2;
    const size_t new_total_size = sizeof(PlannedAction) * new_cap;
    PlannedAction* new_actions = (PlannedAction*)realloc(pl->actions, new_total_size);
    if (!new_actions)
      return;

    pl->actions = new_actions;
    pl->actions_cap = new_cap;
  }

  memcpy(&pl->actions[pl->actions_len], action, sizeof(PlannedAction));
  pl->actions_len++;
}

static inline void FreePlannedAction(PlannedAction* action) {
  if (!action)
    return;

  if (action->id)
    free(action->id);
}

void FreePlan(Plan* pl) {
  if (!pl)
    return;

  if (pl->actions) {
    for (size_t i = 0; i < pl->actions_len; i++)
      FreePlannedAction(&pl->actions[i]);

    free(pl->actions);
  }

  free(pl);
}
