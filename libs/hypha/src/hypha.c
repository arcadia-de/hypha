#include "hypha.h"

#include <stdio.h>
#include <uv.h>

#include "archive_controller.h"
#include "dir_controller.h"
#include "hypha/controller.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"
#include "package_controller.h"
#include "repository_controller.h"
#include "symlink_controller.h"
#include "template_controller.h"
#include "test_controller.h"

static inline void FailedToRegisterController(const char* name) {
  LOG_FATAL("failed to register %s controller", name);
}

void InitControllers() {
  // Controller Controller (Root)
  {
    static const char* kControllerControllerName = "Controller";
    ControllerConfig config = {};
    Controller* ctrl = RegisterController(kControllerControllerName, config, NULL, NULL);
    if (!ctrl)
      return FailedToRegisterController(kControllerControllerName);
  }

#define FOR_EACH_CONTROLLER(V) \
  V(Test)                      \
  V(Archive)                   \
  V(Directory)                 \
  V(Package)                   \
  V(Repository)                \
  V(Symlink)                   \
  V(Template)

#define INIT_CONTROLLER(Name)                        \
  Controller* Name##_ctrl = New##Name##Controller(); \
  if (!Name##_ctrl)                                  \
    return FailedToRegisterController(#Name);        \
  ControllerInit(Name##_ctrl);

  FOR_EACH_CONTROLLER(INIT_CONTROLLER);
#undef INIT_CONTROLLER
}

void InitHypha(const char* luarocks_dir) {
  InitPackageManagers(luarocks_dir);
  InitControllers();
}
