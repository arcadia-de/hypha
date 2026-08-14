#include "brew_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Brew){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewBrew() {
  return NewPackageManager(kBrewName, NULL, &kBrewConfig, NULL, NULL);
}
