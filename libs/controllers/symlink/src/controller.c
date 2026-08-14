#include <errno.h>
#include <jansson.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/log.h"
#include "symlink_controller.h"

DEFINE_CONTROLLER_OBSERVE_FN(Symlink) {
  if (!desired->spec) {
    LOG_ERROR("desired spec is invalid");
    return kStatusInvalidSpec;
  }

  json_error_t err;
  json_t* root = json_loads(desired->spec, 0, &err);
  if (!root) {
    LOG_ERROR("invalid symlink spec:\n%s", desired->spec);
    LOG_ERROR("error on line %d: %s", err.line, err.text);
    return kStatusInvalidSpec;
  }

  json_t* source = json_object_get(root, "source");
  LOG_DEBUG("source: %s", json_string_value(source));

  json_t* target = json_object_get(root, "target");
  LOG_DEBUG("target: %s", json_string_value(target));

  json_decref(root);
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Symlink) {
  ControllerAction action = kNoAction;

  // TODO(@s0cks): add ASSERT(desired->spec);

  json_error_t err;
  json_t* root = json_loads(desired->spec, 0, &err);
  // TODO(@s0cks): add ASSERT(root) && check err

  json_t* source = json_object_get(root, "source");
  const char* source_path = json_string_value(source);

  struct stat source_stat;
  if (stat(source_path, &source_stat) != 0) {
    LOG_ERROR("source %s does not exist", source_path);
    goto finished;
  }

  json_t* target = json_object_get(root, "target");
  const char* target_path = json_string_value(target);
  struct stat target_stat;
  if (lstat(target_path, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode))
      goto finished;
    LOG_WARN("target %s exists but is not a symbolic link", target_path);
  }

  json_decref(root);
  action = kCreateAction;
finished:
  return action;
}

DEFINE_CONTROLLER_APPLY_FN(Symlink) {
  ControllerStatus status = kStatusInternalError;

  json_error_t err;
  json_t* root = json_loads(desired->spec, 0, &err);
  // TODO(@s0cks): add ASSERT(root) && check err

  json_t* source = json_object_get(root, "source");
  const char* source_path = json_string_value(source);

  json_t* target = json_object_get(root, "target");
  const char* target_path = json_string_value(target);

  if (action == kCreateAction) {
    if (symlink(source_path, target_path) != 0) {
      LOG_ERROR("failed to create symlink: %s", strerror(errno));
      status = kStatusInternalError;
      goto finished;
    }
  }

  status = kStatusOk;
finished:
  return status;
}

DEFINE_CONTROLLER_STATUS_FN(Symlink) {
  ControllerStatus status = kStatusInternalError;

  json_error_t err;
  json_t* root = json_loads(current->spec, 0, &err);
  // TODO(@s0cks): add ASSERT(root) && check err

  json_t* source = json_object_get(root, "source");
  const char* source_path = json_string_value(source);

  struct stat source_stat;
  if (stat(source_path, &source_stat) != 0) {
    LOG_ERROR("source '%s' does not exist", source_path);
    goto finished;
  }

  json_t* target = json_object_get(root, "target");
  const char* target_path = json_string_value(target);

  struct stat target_stat;
  if (lstat(target_path, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode)) {
      status = kStatusOk;
      goto finished;
    }

    LOG_ERROR("target '%s' is not a symlink", target_path);
  }

finished:
  return status;
}

static const ControllerConfig kSymlinkControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = SymlinkObserve,
    .plan = SymlinkPlan,
    .apply = SymlinkApply,
    .destroy = NULL,
    .validate = NULL,
    .diff = NULL,
    .status = SymlinkStatus,
    .rollback = NULL,
    .normalize = NULL,
};
DEFINE_NEW_CONTROLLER(Symlink, SYMLINK_CONTROLLER_KIND);
