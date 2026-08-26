#ifndef HYPHA_TASK_SPEC_H
#define HYPHA_TASK_SPEC_H

#include <jansson.h>
#include <stdint.h>
#include <stdlib.h>

#include "retry_policy.h"
#include "task_policy.h"

typedef int64_t TaskTimeout;
static const TaskTimeout kDefaultTaskTimeout = -1;

typedef struct {
  char** argv;
  size_t argv_len;
  char* shell;
  bool use_default_shell;
  char* workdir;
  char** env;
  size_t env_len;
} ExecSpec;

typedef struct {
  RetryPolicy policy;
  int32_t attempts;
  int32_t delay_ms;
} TaskRetrySpec;

typedef struct {
  ExecSpec exec;
  ExecSpec check;
  bool has_check;
  TaskPolicy policy;
  TaskTimeout timeout;
  TaskRetrySpec retry;
  bool sudo;
} TaskSpec;

bool ParseTaskSpec(json_t* doc, TaskSpec* spec);

#endif  // HYPHA_TASK_SPEC_H
