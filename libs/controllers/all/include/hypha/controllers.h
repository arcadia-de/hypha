#ifndef HYPHA_CONTROLLERS_H
#define HYPHA_CONTROLLERS_H

#include "hypha/archive_controller.h"
#include "hypha/controller.h"
#include "hypha/directory_controller.h"
#include "hypha/package_controller.h"
#include "hypha/package_manager_controller.h"
#include "hypha/repository_controller.h"
#include "hypha/symlink_controller.h"
#include "hypha/template_controller.h"
#include "hypha/test_controller.h"

#define FOR_EACH_CONTROLLER(V) \
  V(Controller)                \
  V(Test)                      \
  V(Archive)                   \
  V(Directory)                 \
  V(Package)                   \
  V(Repository)                \
  V(Symlink)                   \
  V(Template)                  \
  V(PackageManager)

void InitControllers();

#endif  // HYPHA_CONTROLLERS_H
