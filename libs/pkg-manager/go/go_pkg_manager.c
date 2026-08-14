#include "go_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Go){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewGo() {
  return NewPackageManager(kGoName, NULL, &kGoConfig, NULL, NULL);
}
