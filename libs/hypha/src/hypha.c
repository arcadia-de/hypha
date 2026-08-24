#include "hypha.h"

#include <stdio.h>

#include "archive_controller.h"
#include "dir_controller.h"
#include "hypha/controller.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"
#include "hypha/resource_kind.h"
#include "hypha/service_manager.h"
#include "package_controller.h"
#include "repository_controller.h"
#include "symlink_controller.h"
#include "systemd.h"
#include "template_controller.h"
#include "test_controller.h"

static inline void FailedToRegisterController(const char* name) {
  LOG_FATAL("failed to register %s controller", name);
}

#define FOR_EACH_CONTROLLER(V) \
  V(Controller)                \
  V(Test)                      \
  V(Archive)                   \
  V(Directory)                 \
  V(Package)                   \
  V(Repository)                \
  V(Symlink)                   \
  V(Template)

static Controller* NewControllerController() {
  ResourceKind kControllerKind = NewResourceKind("Controller");
  static ControllerConfig kControllerConfig = {};
  return NewController(kControllerKind, kControllerConfig, NULL, NULL);
}

void InitControllers() {
#define INIT_CONTROLLER(Name)                        \
  Controller* Name##_ctrl = New##Name##Controller(); \
  if (!Name##_ctrl)                                  \
    return FailedToRegisterController(#Name);        \
  ControllerInit(Name##_ctrl);

  FOR_EACH_CONTROLLER(INIT_CONTROLLER);
#undef INIT_CONTROLLER
}

static inline void InitServiceManagers() {
  InitSystemDServiceManager();
}

void InitHypha(const char* luarocks_dir) {
  InitPackageManagers(luarocks_dir);
  InitServiceManagers();
  InitControllers();
}
