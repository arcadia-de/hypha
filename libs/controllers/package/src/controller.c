#include "hypha.h"
#include "hypha/log.h"
#include "package_controller.h"

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
DEFINE_NEW_CONTROLLER(Package, PACKAGE_CONTROLLER_KIND);
