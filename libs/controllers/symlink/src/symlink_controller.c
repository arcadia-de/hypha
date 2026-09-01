#include "hypha/symlink_controller.h"

#include <errno.h>
#include <jansson.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/symlink_spec.h"
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

thread_local SymlinkSpec symlink_spec;

static const char kSourceField[] = "source";
static const char kTargetField[] = "target";
DEFINE_CONTROLLER_OBSERVE_FN(Symlink) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kSourceField, &symlink_spec.source, &symlink_spec.source_len)) {
    LOG_ERROR("failed to get source field");
    return kStatusInternalError;
  }

  if (!GetSpecField(observed, kTargetField, &symlink_spec.target, &symlink_spec.target_len)) {
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

  if (!symlink_spec.source || symlink_spec.source_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `source` symlink_spec field");
    return false;
  }

  if (!symlink_spec.target || symlink_spec.target_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `target` symlink_spec field");
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

  struct stat source_stat;
  if (stat(symlink_spec.source, &source_stat) != 0) {
    PlannedAction* action = NewNoPlannedAction(log, desired, "Source `%s` does not exist", symlink_spec.source);
    ASSERT(action);
    return kNoAction;
  }

  struct stat target_stat;
  if (lstat(symlink_spec.target, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode)) {
      PlannedAction* action =
          NewNoPlannedAction(log, desired, "Target `%s` already exists and is a valid symlink", symlink_spec.target);
      // TODO(@s0cks): check symlink dest
      ASSERT(action);
      return kNoAction;
    }

    PlannedAction* action =
        NewNoPlannedAction(log, desired, "Target `%s` already exists but is not a symlink", symlink_spec.target);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewCreatePlannedAction(log, desired, "Target `%s` doesn't exist", symlink_spec.target);
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Symlink) {
  if (symlink(symlink_spec.source, symlink_spec.target) != 0) {
    LOG_ERROR("Failed to create symlink from '%s' to '%s': %s", symlink_spec.source, symlink_spec.target,
              strerror(errno));
    return kStatusInternalError;
  }

  AppliedAction* action = NewCreateAction(ctx->log, ctx->desired, "Symlink `%s` created", ctx->desired->info.name);
  ASSERT(action);
  return kStatusOk;
}

DEFINE_CONTROLLER_STATUS_FN(Symlink) {
  const Resource* current = ctx->current;
  ASSERT(current);

  struct stat source_stat;
  if (stat(symlink_spec.source, &source_stat) != 0) {
    LOG_ERROR("Source '%s' does not exist", symlink_spec.source);
    return kStatusInternalError;
  }

  struct stat target_stat;
  if (lstat(symlink_spec.target, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode))
      return kStatusOk;

    LOG_ERROR("Target '%s' is not a symlink", symlink_spec.target);
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
