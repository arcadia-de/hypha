#include "hypha/symlink_controller.h"

#include <errno.h>
#include <jansson.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/controller_status.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/validation_log.h"

typedef struct {
  char* source;
  size_t source_len;

  char* target;
  size_t target_len;
} SymlinkSpec;

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  Expander expander;
  json_t* source = json_object_get(res->spec.doc, field);
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

thread_local SymlinkSpec spec;

static const char kSourceField[] = "source";
static const char kTargetField[] = "target";
DEFINE_CONTROLLER_OBSERVE_FN(Symlink) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!GetSpecField(desired, kSourceField, &spec.source, &spec.source_len)) {
    LOG_ERROR("failed to get source field");
    return kStatusInternalError;
  }

  if (!GetSpecField(desired, kTargetField, &spec.target, &spec.target_len)) {
    LOG_ERROR("failed to get target field");
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Symlink) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!spec.source || spec.source_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `source` spec field");
    return false;
  }

  if (!spec.target || spec.target_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `target` spec field");
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Symlink) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);
  ASSERT(desired);

  struct stat source_stat;
  if (stat(spec.source, &source_stat) != 0) {
    PlannedAction* action = NewNoPlannedAction(log, desired, "Source `%s` does not exist", spec.source);
    ASSERT(action);
    return kNoAction;
  }

  struct stat target_stat;
  if (lstat(spec.target, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode)) {
      PlannedAction* action =
          NewNoPlannedAction(log, desired, "Target `%s` already exists and is a valid symlink", spec.target);
      // TODO(@s0cks): check symlink dest
      ASSERT(action);
      return kNoAction;
    }

    PlannedAction* action =
        NewNoPlannedAction(log, desired, "Target `%s` already exists but is not a symlink", spec.target);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewCreatePlannedAction(log, desired, "Target `%s` doesn't exist", spec.target);
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Symlink) {
  if (symlink(spec.source, spec.target) != 0) {
    LOG_ERROR("Failed to create symlink from '%s' to '%s': %s", spec.source, spec.target, strerror(errno));
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_STATUS_FN(Symlink) {
  const Resource* current = ctx->current;
  ASSERT(current);

  ASSERT(current);
  struct stat source_stat;
  if (stat(spec.source, &source_stat) != 0) {
    LOG_ERROR("Source '%s' does not exist", spec.source);
    return kStatusInternalError;
  }

  struct stat target_stat;
  if (lstat(spec.target, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode))
      return kStatusOk;

    LOG_ERROR("Target '%s' is not a symlink", spec.target);
    return kStatusInternalError;
  }

  return kStatusInternalError;
}

static const ControllerConfig kSymlinkControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = SymlinkObserve,
    .plan = SymlinkPlan,
    .apply = SymlinkApply,
    .validate = SymlinkValidate,
    .diff = NULL,
    .status = SymlinkStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};
DEFINE_NEW_CONTROLLER(Symlink);
