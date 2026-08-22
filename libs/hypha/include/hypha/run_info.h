#ifndef HYPHA_RUN_INFO_H
#define HYPHA_RUN_INFO_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <string.h>
#include <time.h>
#include <uuid/uuid.h>

#include "hypha.h"

typedef struct {
  uuid_t id;
  OrchestratorRunMode mode;
  struct timespec start;
  struct timespec finish;
  Reason reason;
  bool success;
} RunInfo;

static inline void InitRunInfoWithNewId(RunInfo* info, const OrchestratorRunMode mode) {
  ASSERT(info);
  memset(info, 0, sizeof(RunInfo));
  info->mode = mode;
  uuid_generate_random(info->id);
}

static inline void InitRunInfo(RunInfo* info, const OrchestratorRunMode mode, const uuid_t id) {
  ASSERT(info);
  memset(info, 0, sizeof(RunInfo));
  info->mode = mode;
  uuid_copy(info->id, id);
}

static inline void InitRunInfoWithReason(RunInfo* info, const OrchestratorRunMode mode, const uuid_t id,
                                         const Reason reason) {
  ASSERT(info);
  memset(info, 0, sizeof(RunInfo));
  info->mode = mode;
  uuid_copy(info->id, id);
  memset(info->reason, 0, sizeof(Reason));
  memcpy(info->reason, reason, sizeof(Reason));
}

static inline void RunInfoStart(RunInfo* info) {
  ASSERT(info);
  clock_gettime(CLOCK_REALTIME, &info->start);
  info->success = true;
}

static inline bool RunInfoFinish(RunInfo* info) {
  ASSERT(info);
  clock_gettime(CLOCK_REALTIME, &info->finish);
  return info->success;
}

static inline void FreeRunInfo(RunInfo* info) {
  ASSERT(info);
  uuid_clear(info->id);
  memset(info->reason, 0, sizeof(Reason));
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RUN_INFO_H
