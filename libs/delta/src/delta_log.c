#include "hypha/delta_log.h"

#include <stdint.h>
#include <string.h>

#include "hypha/assertions.h"
#include "hypha/log.h"

void InitDeltaLog(DeltaLog* dlog, const size_t init_cap) {
  ASSERT(dlog);
  ASSERT(init_cap > 0);

  const size_t total_size = sizeof(Delta) * init_cap;
  Delta* deltas = (Delta*)malloc(total_size);
  if (!deltas) {
    LOG_ERROR("failed to allocate deltas of cap %zu", init_cap);
    return;
  }

  memset(deltas, 0, total_size);
  dlog->deltas = deltas;
  dlog->deltas_len = 0;
  dlog->deltas_cap = init_cap;
}

static inline bool EnsureCap(DeltaLog* dlog, const size_t new_len) {
  if (new_len < dlog->deltas_cap)
    return true;

  const size_t new_cap = dlog->deltas_cap + new_len;
  const size_t total_size = sizeof(Delta) * new_cap;
  Delta* new_deltas = (Delta*)realloc(dlog->deltas, total_size);
  if (!new_deltas)
    return false;

  dlog->deltas = new_deltas;
  dlog->deltas_cap = new_cap;
  return true;
}

Delta* NewDelta(DeltaLog* dlog) {
  ASSERT(dlog);
  EnsureCap(dlog, dlog->deltas_len + 1);
  Delta* delta = &dlog->deltas[dlog->deltas_len];
  dlog->deltas_len++;
  memset(delta, 0, sizeof(Delta));
  return delta;
}

void AppendDeltas(DeltaLog* dst, Delta* deltas, const size_t num_deltas) {
  ASSERT(dst);
  ASSERT(deltas);
  ASSERT(num_deltas > 0);
  EnsureCap(dst, dst->deltas_len + num_deltas);
  const size_t total_size = sizeof(Delta) * num_deltas;
  memcpy(&dst->deltas[dst->deltas_len], &deltas[0], total_size);
  dst->deltas_len += num_deltas;
}

void VisitAllDeltas(DeltaLog* dlog, VisitDeltaFn fn, void* data) {
  if (!dlog || !dlog->deltas)
    return;

  for (size_t i = 0; i < dlog->deltas_len; i++) {
    const Delta* delta = &dlog->deltas[i];
    if (!fn((uint64_t)i, delta, data))
      return;
  }
}

static inline void FreeDelta(Delta* delta) {
  ASSERT(delta);
  // do something?
}

void FreeDeltaLog(DeltaLog* dlog) {
  if (!dlog)
    return;

  if (dlog->deltas) {
    for (size_t i = 0; i < dlog->deltas_len; i++)
      FreeDelta(&dlog->deltas[i]);
    free(dlog->deltas);
  }

  dlog->deltas = NULL;
  dlog->deltas_len = dlog->deltas_cap = 0;
}
