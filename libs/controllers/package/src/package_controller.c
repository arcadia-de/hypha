#include "hypha/package_controller.h"

#include "hypha.h"
#include "hypha/log.h"

DEFINE_CONTROLLER_OBSERVE_FN(Package) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Package) {
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(Package) {
  return kStatusOk;
}

static const ControllerConfig kPackageControllerConfig = {
    .observe = PackageObserve,
    .plan = PackagePlan,
    .apply = PackageApply,
};
DEFINE_NEW_CONTROLLER(Package);
