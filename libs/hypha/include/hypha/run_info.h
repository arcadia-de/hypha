#ifndef HYPHA_RUN_INFO_H
#define HYPHA_RUN_INFO_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <string.h>
#include <time.h>
#include <uuid/uuid.h>

#include "hypha.h"
#include "hypha/assertions.h"
#include "hypha/controller_status.h"
#include "hypha/reason.h"
#include "hypha/run_mode.h"

typedef struct {
  uuid_t id;
  OrchestratorRunMode mode;
  struct timespec start;
  struct timespec finish;
  Reason reason;
  ControllerStatus status;
} RunInfo;

static inline void RunInfoStart(RunInfo* info) {
  ASSERT(info);
  clock_gettime(CLOCK_REALTIME, &info->start);
  info->status = kStatusOk;
}

static inline bool RunInfoFinish(RunInfo* info) {
  ASSERT(info);
  clock_gettime(CLOCK_REALTIME, &info->finish);
  return info->status == kStatusOk;
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
