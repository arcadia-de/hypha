#include "eget_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Eget){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewEget() {
  return NewPackageManager(kEgetName, NULL, &kEgetConfig, NULL, NULL);
}
