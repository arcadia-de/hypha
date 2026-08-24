#include "hypha/action_log.h"

#include <string.h>

#include "hypha/assertions.h"
#include "hypha/log.h"

void InitActionLog(ActionLog* alog, const size_t init_cap) {
  ASSERT(alog);
  if (init_cap > 0) {
    const size_t total_size = sizeof(AppliedAction) * init_cap;
    AppliedAction* new_actions = (AppliedAction*)malloc(total_size);
    LOG_FATAL_IF(!new_actions, "failed to allocate ActionLog of %zu", init_cap);
    memset(new_actions, 0, total_size);
    alog->actions = new_actions;
    alog->actions_len = 0;
    alog->actions_cap = init_cap;
  }
}

static inline void EnsureLength(ActionLog* alog, const size_t new_len) {
  ASSERT(alog);
  if (new_len >= alog->actions_cap) {
    const size_t new_cap = (alog->actions_cap + new_len) * 2;
    const size_t total_size = sizeof(AppliedAction) * new_cap;
    AppliedAction* new_actions = (AppliedAction*)malloc(total_size);
    LOG_FATAL_IF(!new_actions, "failed to allocate new ActionLog of size %zu", new_cap);
    alog->actions = new_actions;
    alog->actions_cap = new_cap;
  }
}

AppliedAction* NewAppliedAction(ActionLog* alog) {
  ASSERT(alog);
  EnsureLength(alog, alog->actions_len + 1);
  ASSERT(alog->actions_cap > (alog->actions_len + 1));
  AppliedAction* next = &alog->actions[alog->actions_len];
  alog->actions_len++;
  memset(next, 0, sizeof(AppliedAction));
  return next;
}

void AppendAppliedAction(ActionLog* alog, AppliedAction* rhs) {
  ASSERT(alog);
  EnsureLength(alog, alog->actions_len + 1);
  ASSERT(alog->actions_cap > (alog->actions_len + 1));
  memcpy(&alog->actions[alog->actions_len], rhs, sizeof(AppliedAction));
  alog->actions_len++;
}

void AppendActionLog(ActionLog* alog, ActionLog* rhs) {
  ASSERT(alog);
  ASSERT(rhs);
  ASSERT(rhs->actions);
  EnsureLength(alog, alog->actions_len + rhs->actions_len);
  ASSERT(alog->actions_cap > (alog->actions_len + rhs->actions_len));
  memcpy(&alog->actions[alog->actions_len], &rhs->actions[0], sizeof(AppliedAction) * rhs->actions_len);
  alog->actions_len += rhs->actions_len;
}

void VisitAllActions(ActionLog* alog, VisitActionFn fn, void* data) {
  ASSERT(alog);
  ASSERT(fn);
  for (size_t i = 0; i < alog->actions_len; i++) {
    if (!fn(i, &alog->actions[i], data))
      return;
  }
}

void SortActionLog(ActionLog* alog) {
  ASSERT(alog);
  qsort(alog->actions, alog->actions_len, sizeof(AppliedAction), &CompareAppliedAction);
}

void FreeActionLog(ActionLog* alog, const size_t init_cap) {
  ASSERT(alog);
  if (alog->actions)
    free(alog->actions);

  if (init_cap > 0)
    InitActionLog(alog, init_cap);
}
