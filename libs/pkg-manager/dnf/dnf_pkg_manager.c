#include "dnf_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Dnf){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewDnf() {
  return NewPackageManager(kDnfName, NULL, &kDnfConfig, NULL, NULL);
}
