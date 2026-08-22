#ifndef HYPHA_ACTION_LOG_H
#define HYPHA_ACTION_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <time.h>

#include "hypha.h"

typedef struct {
  uint32_t action;
  struct timespec timestamp;
  Resource* resource;
  Reason reason;
} AppliedAction;

static inline int CompareAppliedAction(const void* lhs, const void* rhs) {
  const AppliedAction* a = (const AppliedAction*)lhs;
  const AppliedAction* b = (const AppliedAction*)rhs;
  if (a->action < b->action) {
    return -1;
  } else if (a->action > b->action) {
    return +1;
  }

  const struct timespec* lhst = &a->timestamp;
  const struct timespec* rhst = &b->timestamp;
  if (lhst->tv_sec < rhst->tv_sec)
    return -1;
  else if (lhst->tv_sec > rhst->tv_sec)
    return +1;

  if (lhst->tv_nsec < rhst->tv_nsec)
    return -1;
  else if (lhst->tv_nsec > rhst->tv_nsec)
    return +1;
  return 0;
}

typedef struct {
  AppliedAction* actions;
  size_t actions_len;
  size_t actions_cap;
} ActionLog;

typedef bool (*VisitActionFn)(const size_t idx, AppliedAction* value, void* data);

void InitActionLog(ActionLog* alog, const size_t init_cap);
AppliedAction* NewAppliedAction(ActionLog* vl);
void AppendAction(ActionLog* vl, AppliedAction* rhs);
void AppendActionLog(ActionLog* vl, ActionLog* rhs);
void VisitAllActions(ActionLog* vl, VisitActionFn fn, void* data);
void SortActionLog(ActionLog* alog);
void FreeActionLog(ActionLog* alog, const size_t init_cap);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_ACTION_LOG_H
