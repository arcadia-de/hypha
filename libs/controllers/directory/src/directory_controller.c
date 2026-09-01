#include "hypha/directory_controller.h"

#include <errno.h>
#include <jansson.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/directory_spec.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/validation_log.h"

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  json_t* source = json_object_get(res->spec.doc, field);
  if (!source || !json_is_string(source))
    return false;

  Expander expander;
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

static inline bool MkdirRecursive(const char* path, const mode_t mode) {
  if (!path || path[0] == '\0')
    return false;

  char buf[PATH_MAX];
  const size_t len = strlen(path);
  if (len >= sizeof(buf))
    return false;

  memcpy(buf, path, len + 1);
  for (char* p = buf + 1; *p; p++) {
    if (*p != '/')
      continue;

    *p = '\0';
    if (mkdir(buf, mode) != 0 && errno != EEXIST)
      return false;
    *p = '/';
  }

  if (mkdir(buf, mode) != 0 && errno != EEXIST)
    return false;

  return true;
}

thread_local DirectorySpec directory_spec;

static const char kTargetField[] = "target";

DEFINE_CONTROLLER_OBSERVE_FN(Directory) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kTargetField, &directory_spec.target, &directory_spec.target_len)) {
    LOG_ERROR("failed to get `%s` field", kTargetField);
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Directory) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!directory_spec.target || directory_spec.target_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` directory_spec field", kTargetField);
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Directory) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  struct stat target_stat;
  if (lstat(directory_spec.target, &target_stat) == 0) {
    if (S_ISDIR(target_stat.st_mode)) {
      PlannedAction* action = NewNoPlannedAction(log, desired, "target `%s` exists", directory_spec.target);
      ASSERT(action);
      return kNoAction;
    }

    PlannedAction* action =
        NewNoPlannedAction(log, desired, "target `%s` exists, but is not a directory", directory_spec.target);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action =
      NewCreatePlannedAction(log, desired, "target `%s` does not exist, creating", directory_spec.target);
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Directory) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!MkdirRecursive(directory_spec.target, 0777)) {
    LOG_ERROR("failed to create `%s` directory: %s", directory_spec.target, strerror(errno));
    return kStatusInternalError;
  }

  DLOG_INFO("created `%s` directory", directory_spec.target);
  AppliedAction* action = NewCreateAction(ctx->log, desired, "`%s` directory created", directory_spec.target);
  ASSERT(action);
  return kStatusOk;
}

DEFINE_CONTROLLER_STATUS_FN(Directory) {
  const Resource* current = ctx->current;
  ASSERT(current);

  struct stat target_stat;
  if (lstat(directory_spec.target, &target_stat) != 0 || !S_ISDIR(target_stat.st_mode)) {
    LOG_ERROR("target `%s` does not exist or is not a directory", directory_spec.target);
    return kStatusInternalError;
  }

  return kStatusOk;
}

static const ControllerConfig kDirectoryControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = DirectoryObserve,
    .plan = DirectoryPlan,
    .apply = DirectoryApply,
    .validate = DirectoryValidate,
    .diff = NULL,
    .status = DirectoryStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};
DEFINE_NEW_CONTROLLER(Directory);
