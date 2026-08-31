#include "hypha.h"

#include <ctype.h>
#include <stdio.h>

#include "bootstrap.h"
#include "hypha/assertions.h"
#include "hypha/controllers.h"
#include "hypha/env.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"
#include "hypha/resource_bootstrap.h"
#include "hypha/resource_kind.h"
#include "hypha/service_manager.h"
#include "systemd.h"

static inline void InitServiceManagers() {
  InitSystemDServiceManager();
}

// Kind name for the Manifest pseudo-resource (see InitHypha below). Shared here as a single
// #define so the Go side (internal/hypha/manifest_resource.go) has exactly one string literal to
// keep in sync with, matching the existing convention of hardcoding kind names like "Controller"
// and "PackageManager" at their point of use rather than routing them through a shared registry.
#define kManifestResourceKindName "Manifest"

void InitHypha(const char* luarocks_dir) {
  // Registered ahead of controller init, on purpose: Manifest resources are graph-visible
  // bookkeeping (RESOURCE_FLAG_SYNTHETIC) representing a discovered manifest itself, not a
  // user-managed resource. They never carry depends_on, never enter kResourcePending, and have
  // no controller by design -- the reconcile loop never has a reason to look one up. Registering
  // the kind before InitControllers() isn't load-bearing (FindOrCreateResourceKind works fine at
  // any point), but it means the kind exists from the moment the runtime boots rather than only
  // after the first manifest is discovered, which is one less thing to reason about for anything
  // that walks VisitAllResourceKinds early (`hypha info`, kind-driven CLI command registration).
  if (FindOrCreateResourceKind(kManifestResourceKindName) == kInvalidResourceKind)
    LOG_FATAL("failed to register `%s` resource kind", kManifestResourceKindName);

  InitControllers();
  InitPackageManagers(luarocks_dir);
  InitServiceManagers();
}

static inline void ToLowerCStr(char* dst, const char* src, const size_t dst_size) {
  size_t i = 0;
  for (; src[i] != '\0' && i < dst_size - 1; i++)
    dst[i] = (char)tolower((unsigned char)src[i]);
  dst[i] = '\0';
}

// Kind name for the Manifest pseudo-resource (see InitHypha below). Shared here as a single
// #define so the Go side (internal/hypha/manifest_resource.go) has exactly one string literal to
// keep in sync with, matching the existing convention of hardcoding kind names like "Controller"
// and "PackageManager" at their point of use rather than routing them through a shared registry.
#define kManifestResourceKindName "Manifest"

#define HYPHA_MAX_CORE_RESOURCE_DEFS 64

// clang-format off
static const char kDirectoryDocs[]      = "https://github.com/arcadia-de/hypha/wiki/BuiltinResources-Directories";
static const char kControllerDocs[]     = "";
static const char kTestDocs[]           = "";
static const char kArchiveDocs[]        = "https://github.com/arcadia-de/hypha/wiki/BuiltinResources-Archives";
static const char kPackageDocs[]        = "https://github.com/arcadia-de/hypha/wiki/BuiltinResources-Packages";
static const char kPackageManagerDocs[] = "";
static const char kRepositoryDocs[]     = "https://github.com/arcadia-de/hypha/wiki/BuiltinResources-Repositories";
static const char kSymlinkDocs[]        = "https://github.com/arcadia-de/hypha/wiki/BuiltinResources-Symlinks";
static const char kTemplateDocs[]       = "https://github.com/arcadia-de/hypha/wiki/BuiltinResources-Templates";
static const char kTaskDocs[]           = "";
// clang-format on

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
                              .docs = k##Name##Docs,                   \
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
