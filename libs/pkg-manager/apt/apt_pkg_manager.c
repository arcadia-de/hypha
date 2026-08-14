#include "apt_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Apt){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewApt() {
  return NewPackageManager(kAptName, NULL, &kAptConfig, NULL, NULL);
}
