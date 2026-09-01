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

void AppendPlan(Plan* pl, const Plan* rhs) {
  if (!pl || !rhs)
    return;

  if ((pl->actions_len + rhs->actions_len) >= pl->actions_cap) {
    const size_t new_cap = pl->actions_cap + rhs->actions_cap;
    const size_t total_size = sizeof(PlannedAction) * new_cap;
    PlannedAction* new_actions = (PlannedAction*)realloc(pl->actions, total_size);
    if (!new_actions)
      return;

    pl->actions = new_actions;
    pl->actions_cap = new_cap;
  }

  memcpy(&pl->actions[pl->actions_len], &rhs->actions[0], sizeof(PlannedAction) * rhs->actions_len);
  pl->actions_len++;
}

PlannedAction* NewPlannedAction(Plan* pl) {
  ASSERT(pl);
  if (!pl)
    return NULL;

  if ((pl->actions_len + 1) >= pl->actions_cap) {
    const size_t new_cap = pl->actions_cap * 2;
    const size_t new_total_size = sizeof(PlannedAction) * new_cap;
    PlannedAction* new_actions = (PlannedAction*)realloc(pl->actions, new_total_size);
    if (!new_actions)
      return NULL;

    pl->actions = new_actions;
    pl->actions_cap = new_cap;
  }

  PlannedAction* pa = &pl->actions[pl->actions_len];
  pl->actions_len++;
  memset(pa, 0, sizeof(PlannedAction));
  return pa;
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

void SortPlan(Plan* pl, PlannedActionComparator compare) {
  ASSERT(pl);
  ASSERT(compare);
  qsort(pl->actions, pl->actions_len, sizeof(PlannedAction), compare);
}

static inline void FreePlannedAction(PlannedAction* action) {
  if (!action)
    return;
}

void FreePlan(Plan* pl) {
  if (!pl)
    return;

  if (pl->actions) {
    for (size_t i = 0; i < pl->actions_len; i++)
      FreePlannedAction(&pl->actions[i]);

    free(pl->actions);
  }

  // NOTE: `Plan` is caller-owned storage -- every real usage embeds it directly in a
  // longer-lived struct (e.g. `orc->plan`, `task->plan`) and calls `InitPlan(&x.plan, cap)`
  // on it directly, mirroring `InitValidationLog`/`FreeValidationLog`. This used to also
  // `free(pl)` itself, as if `pl` were heap-allocated, which crashes (or worse, silently
  // corrupts the allocator) the moment anyone calls `FreePlan` on a stack- or embedded-`Plan`
  // -- which is every real call site. There is no `NewPlan()` anywhere that would justify
  // treating `pl` itself as owned by this function.
  pl->actions = NULL;
  pl->actions_len = 0;
  pl->actions_cap = 0;
}
