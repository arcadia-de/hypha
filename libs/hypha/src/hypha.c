#include "hypha.h"

#include <ctype.h>
#include <stdio.h>

#include "archive_controller.h"
#include "bootstrap.h"
#include "dir_controller.h"
#include "hypha/assertions.h"
#include "hypha/controller.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"
#include "hypha/resource_bootstrap.h"
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

  { LOG_FATAL_IF(NewResourceKind("PackageManager") == kInvalidResourceKind, "failed to create PackageManager kind"); }
}

static inline void ToLowerCStr(char* dst, const char* src, const size_t dst_size) {
  size_t i = 0;
  for (; src[i] != '\0' && i < dst_size - 1; i++)
    dst[i] = (char)tolower((unsigned char)src[i]);
  dst[i] = '\0';
}

#define HYPHA_MAX_CORE_RESOURCE_DEFS 64

void BootstrapHyphaCoreResources(ResourceGraph* graph) {
  if (!graph)
    return;

  CoreResourceDef defs[HYPHA_MAX_CORE_RESOURCE_DEFS];
  char names[HYPHA_MAX_CORE_RESOURCE_DEFS][96];
  size_t n = 0;

#define ADD_CONTROLLER_DEF(Name)                                       \
  ASSERT(n < HYPHA_MAX_CORE_RESOURCE_DEFS);                            \
  ToLowerCStr(names[n], #Name, sizeof(names[n]));                      \
  strncat(names[n], "-ctrl", sizeof(names[n]) - strlen(names[n]) - 1); \
  defs[n] = (CoreResourceDef){.kind = "Controller",                    \
                              .name = names[n],                        \
                              .ns = kCoreResourceNamespace,            \
                              .provides = #Name,                       \
                              .flags = kResourceFlagStatic};           \
  n++;

  FOR_EACH_CONTROLLER(ADD_CONTROLLER_DEF)
#undef ADD_CONTROLLER_DEF

#define ADD_PACKAGE_MANAGER_DEF(Name)                                \
  ASSERT(n < HYPHA_MAX_CORE_RESOURCE_DEFS);                          \
  ToLowerCStr(names[n], #Name, sizeof(names[n]));                    \
  strncat(names[n], "-pm", sizeof(names[n]) - strlen(names[n]) - 1); \
  defs[n] = (CoreResourceDef){.kind = "PackageManager",              \
                              .name = names[n],                      \
                              .ns = kCoreResourceNamespace,          \
                              .provides = #Name,                     \
                              .flags = kResourceFlagStatic};         \
  n++;

  FOR_EACH_PACKAGE_MANAGER(ADD_PACKAGE_MANAGER_DEF)
#undef ADD_PACKAGE_MANAGER_DEF

  if (!BootstrapCoreResources(graph, defs, n))
    LOG_ERROR("failed to bootstrap core resources into resource graph");
}

#undef HYPHA_MAX_CORE_RESOURCE_DEFS
#undef FOR_EACH_CONTROLLER
