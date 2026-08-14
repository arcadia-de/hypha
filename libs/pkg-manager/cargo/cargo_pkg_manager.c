#include "cargo_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Cargo){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewCargo() {
  return NewPackageManager(kCargoName, NULL, &kCargoConfig, NULL, NULL);
}
