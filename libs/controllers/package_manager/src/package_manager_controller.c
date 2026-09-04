#include "hypha/package_manager_controller.h"

#include <jansson.h>
#include <string.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/annotation.h"
#include "hypha/controller.h"
#include "hypha/controller_status.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/package_manager_spec.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/resource_bootstrap.h"
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

thread_local PackageManagerSpec package_manager_spec;

static const char kTypeField[] = "type";

DEFINE_CONTROLLER_OBSERVE_FN(PackageManager) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kTypeField, &package_manager_spec.type, &package_manager_spec.type_len)) {
    LOG_ERROR("failed to get `%s` field", kTypeField);
    return kStatusInternalError;
  }

  return kStatusOk;
}

static inline ControllerStatus PackageManagerNormalize(NormalizeContext* ctx, void* data) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!package_manager_spec.type || package_manager_spec.type_len == 0) {
    LOG_ERROR("failed to normalize `%s`: `%s` field is missing", desired->info.name, kTypeField);
    return kStatusInternalError;
  }

  PackageManager* mgr = FindPackageManager(package_manager_spec.type);
  if (!mgr) {
    LOG_ERROR("`%s` is not a registered package manager", package_manager_spec.type);
    return kStatusInternalError;
  }

  AnnotationValue value;
  memset(value, 0, sizeof(AnnotationValue));
  const char* name = GetPackageManagerName(mgr);
  strncpy(value, name, sizeof(AnnotationValue) - 1);

  if (!ResourceHasAnnotationV(desired, &value))
    ResourcePushAnnotation(desired, &kProvidesAnnotationKey, &value);

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(PackageManager) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!package_manager_spec.type || package_manager_spec.type_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` package_manager_spec field", kTypeField);
    return false;
  }

  if (!FindPackageManager(package_manager_spec.type)) {
    NewFailedValidationResult(log, desired, "`%s` is not a registered package manager", package_manager_spec.type);
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(PackageManager) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  PackageManager* mgr = FindPackageManager(package_manager_spec.type);
  if (!mgr || access(GetPackageManagerPath(mgr), X_OK) != 0) {
    PlannedAction* action =
        NewNoPlannedAction(log, desired, "`%s` backend is not available on this host", package_manager_spec.type);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewNoPlannedAction(log, desired, "`%s` backend is available", package_manager_spec.type);
  ASSERT(action);
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(PackageManager) {
  return kStatusOk;
}

// Status and Diff ask the same question here -- "is `type` a registered, usable backend" --
// so they share one implementation, the same shape used for Package's Status/Diff. `dlog` is
// NULL from Status (plain pass/fail, logs failures normally) and non-NULL from Diff (records
// the same finding as a Delta).
static inline ControllerStatus CheckPackageManagerAvailable(const Resource* observed, DeltaLog* dlog) {
  ASSERT(observed);

  PackageManager* mgr = FindPackageManager(package_manager_spec.type);
  if (!mgr) {
    if (dlog) {
      NewNoDelta(dlog, "`%s` is not a registered package manager", package_manager_spec.type);
    } else {
      LOG_ERROR("`%s` is not a registered package manager", package_manager_spec.type);
    }
    return kStatusInternalError;
  }

  if (access(GetPackageManagerPath(mgr), X_OK) != 0) {
    if (dlog) {
      NewNoDelta(dlog, "`%s` backend is not available on this host", package_manager_spec.type);
    } else {
      LOG_ERROR("`%s` backend is not available on this host", package_manager_spec.type);
    }
    return kStatusInternalError;
  }

  if (dlog)
    NewNoDelta(dlog, "`%s` backend is available", package_manager_spec.type);
  return kStatusOk;
}

static inline ControllerStatus PackageManagerCheckStatus(StatusContext* ctx, void* data) {
  return CheckPackageManagerAvailable(ctx->current, NULL);
}

static inline ControllerStatus PackageManagerCheckDiff(DiffContext* ctx, void* data) {
  return CheckPackageManagerAvailable(ctx->observed, ctx->log);
}

static const ControllerConfig kPackageManagerControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = PackageManagerObserve,
    .plan = PackageManagerPlan,
    .apply = PackageManagerApply,
    .validate = PackageManagerValidate,
    .diff = PackageManagerCheckDiff,
    .status = PackageManagerCheckStatus,
    .rollback = NULL,
    .normalize = PackageManagerNormalize,
    .destroy = NULL,
};

static ResourceKind kPackageManagerKind = kInvalidResourceKind;

ResourceKind GetPackageManagerResourceKind() {
  return kPackageManagerKind;
}

Controller* NewPackageManagerController() {
  kPackageManagerKind = NewResourceKind(kPackageManagerControllerKindName);
  if (kPackageManagerKind == kInvalidResourceKind)
    return NULL;

  const char* aliases[8] = {
      "packge-manager", "package-managers", "packagemanager", "packagemanagers",
      "pkg-manager",    "pkg-managers",     "pkg-mgr",        "pkg-mgrs",
  };
  return NewController(kPackageManagerKind, kPackageManagerControllerConfig, aliases, 8, NULL, NULL);
}
