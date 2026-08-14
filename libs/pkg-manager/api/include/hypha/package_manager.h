#ifndef HYPHA_PACKAGE_MANAGER_H
#define HYPHA_PACKAGE_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#define FOR_EACH_PACKAGE_STATUS(V) \
  V(Installed)                     \
  V(Skipped)                       \
  V(Uninstalled)                   \
  V(Error)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) kPackage##Name,
  FOR_EACH_PACKAGE_STATUS(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfPackageStatuses,
} PackageStatus;
// clang-format on

static inline const char* PackageStatusName(const PackageStatus rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case kPackage##Name:        \
    return #Name;
    FOR_EACH_PACKAGE_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

typedef struct _PackageManager PackageManager;

typedef PackageStatus (*PackageManagerInstallFn)(PackageManager* pm, const char* name, void* data);

typedef PackageStatus (*PackageManagerStatusFn)(PackageManager* pm, const char* name, void* data);

typedef PackageStatus (*PackageManagerUninstallFn)(PackageManager* pm, const char* name, void* data);

typedef struct {
  PackageManagerInstallFn install;
  PackageManagerStatusFn status;
  PackageManagerUninstallFn uninstall;
} PackageManagerConfig;

PackageManager* NewPackageManager(const char* name, const char* bin, PackageManagerConfig config, void* data,
                                  void (*free_data)(void*));
PackageManager* FindPackageManager(const char* name);
PackageManager* GetPackageManagerAt(const uint64_t idx);
bool IsPackageManagerNamed(const PackageManager* pm, const char* name);
uint64_t GetNumberOfPackageManagers();
const char* GetPackageManagerName(const PackageManager*);
const char* GetPackageManagerPath(const PackageManager*);
void FreePackageManager(PackageManager* rhs);

PackageStatus PackageManagerStatus(PackageManager* mgr, const char* pkg);
PackageStatus PackageManagerInstall(PackageManager* mgr, const char* pkg);
PackageStatus PackageManagerUninstall(PackageManager* mgr, const char* pkg);

int ExecPackageManager(PackageManager* mgr, const char** args, const uint64_t num_args, const bool root);

#define _PACKAGE_MANAGER_FN(Name, Fn) \
  static inline PackageStatus Name##Fn(PackageManager* mgr, const char* pkg, void* data)

#define PACKAGE_MANAGER_INSTALL_FN(Name)   _PACKAGE_MANAGER_FN(Name, Install)
#define PACKAGE_MANAGER_STATUS_FN(Name)    _PACKAGE_MANAGER_FN(Name, Status)
#define PACKAGE_MANAGER_UNINSTALL_FN(Name) _PACKAGE_MANAGER_FN(Name, Uninstall)

#endif  // HYPHA_PACKAGE_MANAGER_H
