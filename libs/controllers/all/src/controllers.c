#include "hypha/controllers.h"

#include "hypha/log.h"

static Controller* NewControllerController() {
  ResourceKind kControllerKind = NewResourceKind("Controller");
  static ControllerConfig kControllerConfig = {};
  return NewController(kControllerKind, kControllerConfig, NULL, NULL);
}

void InitControllers() {
#define INIT_CONTROLLER(Name)                                           \
  Controller* Name##_ctrl = New##Name##Controller();                    \
  if (!Name##_ctrl) {                                                   \
    LOG_FATAL("failed to create controller for `%s` resources", #Name); \
    return;                                                             \
  }                                                                     \
  ControllerInit(Name##_ctrl);

  FOR_EACH_CONTROLLER(INIT_CONTROLLER);
#undef INIT_CONTROLLER
}
