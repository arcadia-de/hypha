#ifndef HYPHA_DIFF_LOG_H
#define HYPHA_DIFF_LOG_H

#include <stdint.h>
#include <stdlib.h>

#include "hypha/delta.h"

typedef struct {
  Delta* deltas;
  size_t deltas_len;
  size_t deltas_cap;
} DeltaLog;

void InitDeltaLog(DeltaLog* dlog, const size_t init_cap);
Delta* NewDelta(DeltaLog* dlog);
void AppendDelta(DeltaLog* dst, Delta* src);
void AppendDeltaLog(DeltaLog* dst, DeltaLog* src);

typedef bool (*VisitDeltaFn)(uint64_t, const Delta*, void*);
void VisitAllDeltas(DeltaLog* dlog, VisitDeltaFn fn, void* data);

void FreeDeltaLog(DeltaLog* dlog);

#endif  // HYPHA_DIFF_LOG_H
