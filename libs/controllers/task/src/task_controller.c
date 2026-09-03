#include "hypha/task_controller.h"

#include <string.h>
#include <time.h>
#include <xxhash.h>

#include "exec_spec.h"
#include "hypha/action_log.h"
#include "hypha/controller.h"
#include "hypha/controller_action.h"
#include "hypha/controller_status.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/process.h"
#include "hypha/task_spec.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

thread_local TaskSpec task;
thread_local int last_check_status = -1;

static inline void FreeExecSpecFields(ExecSpec* spec) {
  if (!spec)
    return;

  for (size_t i = 0; i < spec->argv_len; i++)
    free(spec->argv[i]);

  free(spec->argv);
  free(spec->shell);
  free(spec->workdir);
  for (size_t i = 0; i < spec->env_len; i++)
    free(spec->env[i]);

  free(spec->env);
}

static inline void FreeTaskSpec(TaskSpec* spec) {
  if (!spec)
    return;

  FreeExecSpecFields(&spec->exec);
  FreeExecSpecFields(&spec->check);
}

static inline bool HasExec(const TaskSpec* spec) {
  return spec->exec.argv != NULL && spec->exec.argv_len > 0;
}

static inline void JoinArgs(ExecSpec* spec, char** result) {
  ASSERT(spec);

  size_t total_len = 3;
  for (size_t i = 0; i < spec->argv_len; i++)
    total_len += strlen(spec->argv[i]) + 1;

  char* command = (char*)malloc(sizeof(char) * total_len);
  char* current = command;
  for (size_t i = 0; i < spec->argv_len; i++) {
    const size_t arg_len = strlen(spec->argv[i]);
    memcpy(current, spec->argv[i], arg_len);
    current += arg_len;
    if (i < (spec->argv_len - 1)) {
      (*current) = ' ';
      current += 1;
    }
  }
  (*current) = '\0';

  (*result) = command;
}

static inline void OnProcessOut(Process* p, const char* message) {
  ASSERT(p);
  ASSERT(message);
  LOG_INFO("%s", message);
}

static inline void OnProcessErr(Process* p, const char* message) {
  ASSERT(p);
  ASSERT(message);
  LOG_ERROR("%s", message);
}

static inline void InitProcess(ExecSpec* spec, Process* p, bool* out_owns_command) {
  ASSERT(spec);
  ASSERT(p);
  ASSERT(out_owns_command);

  memset(p, 0, sizeof(Process));
  p->root = task.sudo;
  p->timeout = (int)task.timeout;
  p->out = &OnProcessOut;
  p->err = &OnProcessErr;

  const char* shell = ResolveShellPath(spec);
  if (shell != NULL) {
    char* command = NULL;
    JoinArgs(spec, &command);

    p->bin = shell;
    p->num_args = 2;
    p->args = (const char**)malloc(sizeof(char*) * 2);
    LOG_FATAL_IF(!p->args, "failed to allocate process args");
    p->args[0] = "-c";
    p->args[1] = command;
    (*out_owns_command) = true;
    return;
  }

  ASSERT(spec->argv != NULL && spec->argv_len > 0);
  p->bin = spec->argv[0];
  (*out_owns_command) = false;

  if (spec->argv_len <= 1) {
    p->args = NULL;
    p->num_args = 0;
    return;
  }

  p->num_args = spec->argv_len - 1;
  p->args = (const char**)malloc(sizeof(char*) * p->num_args);
  LOG_FATAL_IF(!p->args, "failed to allocate process args");
  for (size_t i = 0; i < p->num_args; i++)
    p->args[i] = spec->argv[i + 1];
}

static inline void FreeProcessArgs(Process* p, const bool owns_command) {
  if (!p || !p->args)
    return;
  if (owns_command)
    free((void*)p->args[1]);
  free((void*)p->args);
}

static inline int RunCheck(ExecSpec* spec) {
  bool owns_command = false;
  Process proc;
  InitProcess(spec, &proc, &owns_command);
  const int status = ExecProcess(&proc);
  FreeProcessArgs(&proc, owns_command);
  return status;
}

static inline int RunExecWithRetry(ExecSpec* spec, const TaskRetrySpec* retry) {
  const int32_t max_attempts = (retry->policy == kRetryPolicyAlways && retry->attempts > 1) ? retry->attempts : 1;

  int status = -1;
  for (int32_t attempt = 1; attempt <= max_attempts; attempt++) {
    bool owns_command = false;
    Process proc;
    InitProcess(spec, &proc, &owns_command);
    status = ExecProcess(&proc);
    FreeProcessArgs(&proc, owns_command);

    if (status == 0)
      break;

    if (attempt < max_attempts) {
      DLOG_WARN("task exec failed (exit %d), retrying (attempt %d/%d)...", status, attempt + 1, max_attempts);
      if (retry->delay_ms > 0) {
        const struct timespec delay = {
            .tv_sec = retry->delay_ms / 1000,
            .tv_nsec = (long)(retry->delay_ms % 1000) * 1000000L,
        };
        nanosleep(&delay, NULL);
      }
    }
  }
  return status;
}

DEFINE_CONTROLLER_OBSERVE_FN(Task) {
  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Task) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  bool ok = true;

  if (!HasExec(&task)) {
    NewFailedValidationResult(log, desired, "Task `%s` has no `exec.command` to run", desired->info.name);
    ok = false;
  }

  if (task.has_check && (task.check.argv == NULL || task.check.argv_len == 0)) {
    NewFailedValidationResult(log, desired, "Task `%s` `check.command` is empty", desired->info.name);
    ok = false;
  }

  if (task.policy == kTaskPolicyAlways && task.has_check) {
    NewWarningValidationResult(log, desired,
                               "Task `%s` has policy `Always` and a `check` -- `check` is never consulted "
                               "for this policy and will be ignored",
                               desired->info.name);
  }

  if (last_check_status > 0) {
    NewWarningValidationResult(log, desired, "Task `%s` check returned non-zero exit code %d, exec will run",
                               desired->info.name, last_check_status);
  }

  return ok;
}

DEFINE_CONTROLLER_PLAN_FN(Task) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  FreeTaskSpec(&task);
  memset(&task, 0, sizeof(TaskSpec));
  last_check_status = -1;

  if (!ParseTaskSpec(desired->spec.doc, &task)) {
    LOG_ERROR("failed to parse task spec for `%s`", desired->info.name);
    PlannedAction* action = NewFailedPlannedAction(log, desired, "failed to parse task spec");
    ASSERT(action);
    return kFailedAction;
  }

  const uint64_t current_hash =
      desired->spec.raw ? (uint64_t)XXH3_64bits(desired->spec.raw, strlen(desired->spec.raw)) : 0;
  desired->spec.hash = current_hash;

  const Resource* current = ctx->current;
  const bool has_prior_state = current != NULL && current->info.name != NULL;
  const uint64_t previous_hash = has_prior_state ? current->spec.hash : 0;

  bool should_run = false;
  const char* skip_reason = "no action required";
  switch (task.policy) {
    case kTaskPolicyAlways:
      should_run = true;
      break;
    case kTaskPolicyOnce:
      should_run = !has_prior_state;
      skip_reason = "policy `Once` has already run for this Task";
      break;
    case kTaskPolicyOnChange:
    default:
      should_run = !has_prior_state || current_hash != previous_hash;
      skip_reason = "no changes detected since last run";
      break;
  }

  if (should_run && task.policy != kTaskPolicyAlways && task.has_check) {
    last_check_status = RunCheck(&task.check);
    if (last_check_status == 0) {
      should_run = false;
      skip_reason = "check already satisfied (exit 0)";
    }
  }

  if (!should_run) {
    PlannedAction* action = NewNoPlannedAction(log, desired, "%s", skip_reason);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewCreatePlannedAction(log, desired, "Task `%s` will run (policy=%s)", desired->info.name,
                                                 TaskPolicyToCStr(task.policy));
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Task) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!HasExec(&task)) {
    AppliedAction* action =
        NewFailedAction(ctx->log, desired, "Task `%s` has no `exec.command` to run", desired->info.name);
    ASSERT(action);
    return kStatusUnsupported;
  }

  DLOG_INFO("executing task `%s`...", desired->info.name);
  const int status = RunExecWithRetry(&task.exec, &task.retry);
  DLOG_INFO("task `%s` exit status: %d", desired->info.name, status);

  if (status != 0) {
    AppliedAction* action =
        NewFailedAction(ctx->log, desired, "Task `%s` exited with code %d", desired->info.name, status);
    ASSERT(action);
    return kStatusInternalError;
  }

  AppliedAction* action = NewCreateAction(ctx->log, desired, "Task `%s` returned zero exit code", desired->info.name);
  ASSERT(action);
  return kStatusOk;
}

static const ControllerConfig kTaskControllerConfig = {
    .observe = &TaskObserve,
    .plan = &TaskPlan,
    .validate = &TaskValidate,
    .apply = &TaskApply,
};
static ResourceKind kTaskKind = kInvalidResourceKind;

ResourceKind GetTaskResourceKind() {
  return kTaskKind;
}

Controller* NewTaskController() {
  kTaskKind = NewResourceKind(kTaskControllerKindName);
  if (kTaskKind == kInvalidResourceKind)
    return NULL;

  const char* aliases[2] = {
      "task",
      "tasks",
  };
  return NewController(kTaskKind, kTaskControllerConfig, aliases, 2, NULL, NULL);
}
