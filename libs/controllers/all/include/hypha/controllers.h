#ifndef HYPHA_CONTROLLERS_H
#define HYPHA_CONTROLLERS_H

#include "hypha/controller.h"
#include "hypha/directory_controller.h"
#include "hypha/package_controller.h"
#include "hypha/package_manager_controller.h"
#include "hypha/symlink_controller.h"
#include "hypha/task_controller.h"
#include "hypha/template_controller.h"
#include "hypha/test_controller.h"

#ifdef HYPHA_HAS_ARCHIVE
// TODO(@s0cks): need to change FOR_EACH_CONTROLLER
#include "hypha/archive_controller.h"
#endif  // HYPHA_HAS_ARCHIVE

#ifdef HYPHA_HAS_GIT2
// TODO(@s0cks): need to change FOR_EACH_CONTROLLER
#include "hypha/repository_controller.h"
#endif  // HYPHA_HAS_GIT2

#ifdef HYPHA_HAS_CURL
// TODO(@s0cks): need to change FOR_EACH_CONTROLLER
#include "hypha/download_controller.h"
#endif  // HYPHA_HAS_CURL

#define FOR_EACH_CONTROLLER(V) \
  V(Controller)                \
  V(Test)                      \
  V(Task)                      \
  V(Archive)                   \
  V(Directory)                 \
  V(Download)                  \
  V(Package)                   \
  V(Repository)                \
  V(Symlink)                   \
  V(Template)                  \
  V(PackageManager)

void InitControllers();

#endif  // HYPHA_CONTROLLERS_H
