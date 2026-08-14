#include "dir_controller.h"
#include "hypha.h"
#include "hypha/log.h"

DEFINE_CONTROLLER_OBSERVE_FN(Directory) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Directory) {
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(Directory) {
  return kStatusOk;
}

static const ControllerConfig kDirectoryControllerConfig = {
    .observe = DirectoryObserve,
    .plan = DirectoryPlan,
    .apply = DirectoryApply,
};
DEFINE_NEW_CONTROLLER(Directory, DIRECTORY_CONTROLLER_KIND);
