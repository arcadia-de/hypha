#ifndef HYPHA_DIFF_LOG_H
#define HYPHA_DIFF_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/assertions.h"
#include "hypha/delta.h"
#include "hypha/reason.h"
#include "hypha/structured_log.h"

DECLARE_STRUCTURED_LOG(Delta);

#define DEFINE_NEW_DELTA(Name, Level)                                          \
  static inline Delta* New##Name##Delta(DeltaLog* log, const char* fmt, ...) { \
    Delta* delta = NewDelta(log);                                              \
    if (delta) {                                                               \
      delta->change = (Level);                                                 \
      va_list args;                                                            \
      va_start(args, fmt);                                                     \
      vsnprintf(delta->reason, HYPHA_REASON_MAX_LENGTH, fmt, args);            \
      va_end(args);                                                            \
    }                                                                          \
    return delta;                                                              \
  }

DEFINE_NEW_DELTA(Remove, -1);
DEFINE_NEW_DELTA(No, 0);
DEFINE_NEW_DELTA(Add, +1);
DEFINE_NEW_DELTA(Change, 2);
#undef DEFINE_NEW_DELTA

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_DIFF_LOG_H
