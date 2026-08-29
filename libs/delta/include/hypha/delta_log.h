#ifndef HYPHA_DIFF_LOG_H
#define HYPHA_DIFF_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdlib.h>

#include "hypha/assertions.h"
#include "hypha/delta.h"

typedef struct {
  Delta* deltas;
  size_t deltas_len;
  size_t deltas_cap;
} DeltaLog;

void InitDeltaLog(DeltaLog* dlog, const size_t init_cap);
Delta* NewDelta(DeltaLog* dlog);
void AppendDeltas(DeltaLog* dst, Delta* deltas, const size_t num_deltas);
void FreeDeltaLog(DeltaLog* dlog);

typedef bool (*VisitDeltaFn)(uint64_t, const Delta*, void*);
void VisitAllDeltas(DeltaLog* dlog, VisitDeltaFn fn, void* data);

static inline void AppendDelta(DeltaLog* dst, Delta* delta) {
  ASSERT(dst);
  ASSERT(delta);
  return AppendDeltas(dst, delta, 1);
}

static inline void AppendDeltaLog(DeltaLog* dst, DeltaLog* src) {
  ASSERT(dst);
  ASSERT(src);
  ASSERT(src->deltas_len > 0);
  return AppendDeltas(dst, src->deltas, src->deltas_len);
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_DIFF_LOG_H
