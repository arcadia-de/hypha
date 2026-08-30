#ifndef HYPHA_ACTION_LOG_H
#define HYPHA_ACTION_LOG_H

#include "hypha/structured_log.h"
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <time.h>

#include "hypha.h"
#include "hypha/reason.h"
#include "hypha/resource.h"

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

DECLARE_STRUCTURED_LOG(AppliedAction);

void SortActionLog(AppliedActionLog* alog);

#define DEFINE_NEW_ACTION(Name)                                                                                \
  static inline AppliedAction* New##Name##Action(AppliedActionLog* log, Resource* res, const char* fmt, ...) { \
    AppliedAction* action = NewAppliedAction(log);                                                             \
    if (action) {                                                                                              \
      action->action = k##Name##Action;                                                                        \
      action->resource = res;                                                                                  \
      clock_gettime(CLOCK_REALTIME, &action->timestamp);                                                       \
      va_list args;                                                                                            \
      va_start(args, fmt);                                                                                     \
      vsnprintf(action->reason, HYPHA_REASON_MAX_LENGTH, fmt, args);                                           \
      va_end(args);                                                                                            \
    }                                                                                                          \
    return action;                                                                                             \
  }

FOR_EACH_CONTROLLER_ACTION(DEFINE_NEW_ACTION);
#undef DEFINE_NEW_ACTION

// uint32_t action;
// struct timespec timestamp;
// Resource* resource;
// Reason reason;

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_ACTION_LOG_H
