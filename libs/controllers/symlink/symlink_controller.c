#include "symlink_controller.h"

#include <errno.h>
#include <jansson.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/expander.h"
#include "hypha/log.h"

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  Expander expander;
  json_t* source = json_object_get(res->spec.doc, field);
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

DEFINE_CONTROLLER_VALIDATE_FN(Symlink) {
  ControllerValidationResult valid = kValidationkFailed;

  char* source = NULL;
  size_t source_len = 0;
  if (!GetSpecField(desired, "source", &source, &source_len)) {
    snprintf(reason, HYPHA_REASON_MAX_LENGTH, "failed to get 'source' field");
    goto finished;
  }

  char* target = NULL;
  size_t target_len = 0;
  if (!GetSpecField(desired, "target", &target, &target_len)) {
    snprintf(reason, HYPHA_REASON_MAX_LENGTH, "failed to get 'target' field");
    goto finished;
  }

  valid = true;
finished:
  return valid;
}

DEFINE_CONTROLLER_OBSERVE_FN(Symlink) {
  ASSERT(desired);
  json_t* doc = desired->spec.doc;
  ASSERT(doc);

  char* source = NULL;
  size_t source_len = 0;
  if (!GetSpecField(desired, "source", &source, &source_len)) {
    LOG_ERROR("failed to get source field");
    return kStatusInternalError;
  }
  LOG_DEBUG("source: %s", source);

  char* target = NULL;
  size_t target_len = 0;
  if (!GetSpecField(desired, "target", &target, &target_len)) {
    LOG_ERROR("failed to get target field");
    return kStatusInternalError;
  }
  LOG_DEBUG("target: %s", target);
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Symlink) {
  ASSERT(desired);

  char message[PATH_MAX];

  json_t* doc = desired->spec.doc;
  ASSERT(doc);
  ControllerAction action = kNoAction;

  char* source = NULL;
  size_t source_len = 0;
  GetSpecField(desired, "source", &source, &source_len);

  struct stat source_stat;
  if (stat(source, &source_stat) != 0) {
    snprintf(message, HYPHA_REASON_MAX_LENGTH, "source '%s' does not exist", source);
    goto finished;
  }

  char* target = NULL;
  size_t target_len = 0;
  GetSpecField(desired, "target", &target, &target_len);

  struct stat target_stat;
  if (lstat(target, &target_stat) == 0) {
    if (S_ISLNK(target_stat.st_mode)) {
      snprintf(message, HYPHA_REASON_MAX_LENGTH, "target '%s' exists and is a symlink", target);
      goto finished;
    }

    snprintf(message, HYPHA_REASON_MAX_LENGTH, "target %s exists but is not a symlink", target);
    goto finished;
  }

  snprintf(message, HYPHA_REASON_MAX_LENGTH, "target %s does not exist", target);
  action = kCreateAction;
finished:
  return action;
}

DEFINE_CONTROLLER_APPLY_FN(Symlink) {
  ASSERT(desired);
  json_t* doc = desired->spec.doc;
  ASSERT(doc);
  ControllerStatus status = kStatusInternalError;

  char* source = NULL;
  size_t source_len = 0;
  if (!GetSpecField(desired, "source", &source, &source_len)) {
    LOG_ERROR("failed to get source field");
    goto finished;
  }

  char* target = NULL;
  size_t target_len = 0;
  if (!GetSpecField(desired, "target", &target, &target_len)) {
    LOG_ERROR("failed to get target field");
    goto finished;
  }

  if (action == kCreateAction) {
    if (symlink(source, target) != 0) {
      LOG_ERROR("failed to create symlink from '%s' to '%s': %s", source, target, strerror(errno));
      status = kStatusInternalError;
      goto finished;
    }
  }

  status = kStatusOk;
finished:
  return status;
}

DEFINE_CONTROLLER_STATUS_FN(Symlink) {
  ASSERT(current);
  json_t* doc = current->spec.doc;
  ASSERT(doc);
  ControllerStatus status = kStatusInternalError;

  json_t* source = json_object_get(doc, "source");
  const char* source_path = json_string_value(source);

  struct stat source_stat;
  if (stat(source_path, &source_stat) != 0) {
    LOG_ERROR("source '%s' does not exist", source_path);
    goto finished;
  }

  json_t* target = json_object_get(doc, "target");
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
    .validate = SymlinkValidate,
    .diff = NULL,
    .status = SymlinkStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};
DEFINE_NEW_CONTROLLER(Symlink, SYMLINK_CONTROLLER_KIND);
