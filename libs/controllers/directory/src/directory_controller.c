#include "hypha/directory_controller.h"

#include <bits/time.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/validation_log.h"

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res->spec.doc);
  ASSERT(field);
  Expander expander;
  json_t* source = json_object_get(res->spec.doc, field);
  ASSERT(source);
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

DEFINE_CONTROLLER_VALIDATE_FN(Directory) {
  ASSERT(ctx);
  ValidationLog* log = ctx->log;
  Resource* desired = (Resource*)ctx->desired;
  ASSERT(desired);

  bool valid = true;

  char* target = NULL;
  size_t target_len = 0;
  if (!GetSpecField(desired, "target", &target, &target_len)) {
    valid = false;
    NewFailedValidationResult(log, desired, "failed to get `target` spec field");
  }

  return valid;
}

DEFINE_CONTROLLER_OBSERVE_FN(Directory) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Directory) {
  ASSERT(ctx);
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  char* target = NULL;
  size_t target_len = 0;
  LOG_FATAL_IF(!GetSpecField(desired, "target", &target, &target_len), "failed to get `target` field from spec");

  struct stat target_stat;
  if (lstat(target, &target_stat) == 0) {
    if (S_ISDIR(target_stat.st_mode)) {
      PlannedAction* action = NewNoPlannedAction(log, desired, "target `%s` exists", target);
      ASSERT(action);
      return kNoAction;
    }

    PlannedAction* action = NewNoPlannedAction(log, desired, "target `%s` exists, but is not a directory", target);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewCreatePlannedAction(log, desired, "target `%s` does not exist, creating", target);
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Directory) {
  ASSERT(ctx);
  Resource* desired = (Resource*)ctx->desired;
  ASSERT(desired);

  const int perms = 0777;

  char* target = NULL;
  size_t target_len = 0;
  LOG_FATAL_IF(!GetSpecField(desired, "target", &target, &target_len), "failed to get `target` field from spec");

  const int status = mkdir(target, perms);
  if (status != 0) {
    LOG_ERROR("failed to create `%s` directory", target);
    return kStatusInternalError;
  }

  DLOG_INFO("creating `%s` directory", target);
  return kStatusOk;
}

static const ControllerConfig kDirectoryControllerConfig = {
    .observe = DirectoryObserve,
    .validate = DirectoryValidate,
    .plan = DirectoryPlan,
    .apply = DirectoryApply,
};
DEFINE_NEW_CONTROLLER(Directory);
