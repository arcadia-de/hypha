#include "hypha/package_manager_controller.h"

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/package_manager_spec.h"

thread_local PackageManagerSpec spec;

DEFINE_CONTROLLER_OBSERVE_FN(PackageManager) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(PackageManager) {
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(PackageManager) {
  return kStatusOk;
}

static const ControllerConfig kPackageManagerControllerConfig = {
    .observe = PackageManagerObserve,
    .plan = PackageManagerPlan,
    .apply = PackageManagerApply,
};
DEFINE_NEW_CONTROLLER(PackageManager);
