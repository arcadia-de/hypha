#include "hypha/package_controller.h"

#include <jansson.h>
#include <string.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/package_spec.h"
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

thread_local PackageSpec package_spec;

static const char kNameField[] = "name";
static const char kManagerField[] = "manager";

DEFINE_CONTROLLER_OBSERVE_FN(Package) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kNameField, &package_spec.name, &package_spec.name_len)) {
    package_spec.name_len = package_spec.name ? strlen(package_spec.name) : 0;
  }

  if (!GetSpecField(observed, kManagerField, &package_spec.manager, &package_spec.manager_len)) {
    LOG_ERROR("failed to get `%s` field", kManagerField);
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Package) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!package_spec.name || package_spec.name_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` package_spec field", kNameField);
    return false;
  }

  if (!package_spec.manager || package_spec.manager_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` package_spec field", kManagerField);
    return false;
  }

  if (!FindPackageManager(package_spec.manager)) {
    NewFailedValidationResult(log, desired, "`%s` is not a registered package manager", package_spec.manager);
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Package) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  PackageManager* mgr = FindPackageManager(package_spec.manager);
  if (!mgr) {
    PlannedAction* action =
        NewFailedPlannedAction(log, desired, "`%s` is not a registered package manager", package_spec.manager);
    ASSERT(action);
    return kFailedAction;
  }

  const PackageStatus status = PackageManagerStatus(mgr, package_spec.name);
  if (status == kPackageInstalled) {
    PlannedAction* action =
        NewNoPlannedAction(log, desired, "`%s` is already installed via `%s`", package_spec.name, package_spec.manager);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewCreatePlannedAction(log, desired, "`%s` is not installed via `%s` (%s)", package_spec.name,
                                                 package_spec.manager, PackageStatusName(status));
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Package) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  PackageManager* mgr = FindPackageManager(package_spec.manager);
  if (!mgr) {
    LOG_ERROR("`%s` is not a registered package manager", package_spec.manager);
    return kStatusInternalError;
  }

  const PackageStatus status = PackageManagerInstall(mgr, package_spec.name);
  if (status != kPackageInstalled) {
    LOG_ERROR("failed to install `%s` via `%s`: %s", package_spec.name, package_spec.manager,
              PackageStatusName(status));
    return kStatusInternalError;
  }

  AppliedAction* action =
      NewCreateAction(ctx->log, desired, "`%s` installed via `%s`", package_spec.name, package_spec.manager);
  ASSERT(action);
  return kStatusOk;
}

// Status and Diff both boil down to the same question here -- "does the package manager
// report this installed right now" -- so they share one implementation. Unlike Archive/
// Symlink/Template, there isn't a more granular comparison available for Diff to make: a
// package is either installed or it isn't, as far as PackageManagerStatus() can tell us.
// `dlog` is NULL when called from Status (which has no log to write into and just returns
// pass/fail, logging failures the usual way) and non-NULL when called from Diff (which
// records the same finding as a Delta instead).
static inline ControllerStatus CheckPackageInstalled(const Resource* observed, DeltaLog* dlog) {
  ASSERT(observed);
  PackageManager* mgr = FindPackageManager(package_spec.manager);
  if (!mgr) {
    if (dlog) {
      NewNoDelta(dlog, "`%s` is not a registered package manager", package_spec.manager);
    } else {
      LOG_ERROR("`%s` is not a registered package manager", package_spec.manager);
    }
    return kStatusInternalError;
  }

  const PackageStatus status = PackageManagerStatus(mgr, package_spec.name);
  if (status != kPackageInstalled) {
    if (dlog) {
      NewNoDelta(dlog, "`%s` is not installed via `%s`: %s", package_spec.name, package_spec.manager,
                 PackageStatusName(status));
    } else {
      LOG_ERROR("`%s` is not installed via `%s`: %s", package_spec.name, package_spec.manager,
                PackageStatusName(status));
    }
    return kStatusInternalError;
  }

  if (dlog)
    NewNoDelta(dlog, "`%s` is installed via `%s`", package_spec.name, package_spec.manager);

  return kStatusOk;
}

static inline ControllerStatus PackageCheckStatus(StatusContext* ctx, void* data) {
  return CheckPackageInstalled(ctx->current, NULL);
}

static inline ControllerStatus PackageCheckDiff(DiffContext* ctx, void* data) {
  return CheckPackageInstalled(ctx->observed, ctx->log);
}

static const ControllerConfig kPackageControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = PackageObserve,
    .plan = PackagePlan,
    .apply = PackageApply,
    .validate = PackageValidate,
    .diff = PackageCheckDiff,
    .status = PackageCheckStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};
static ResourceKind kPackageKind = kInvalidResourceKind;

ResourceKind GetPackageResourceKind() {
  return kPackageKind;
}

Controller* NewPackageController() {
  kPackageKind = NewResourceKind(kPackageControllerKindName);
  if (kPackageKind == kInvalidResourceKind)
    return NULL;

  const char* aliases[4] = {
      "package",
      "packages",
      "pkg",
      "pkgs",
  };
  return NewController(kPackageKind, kPackageControllerConfig, aliases, 4, NULL, NULL);
}
