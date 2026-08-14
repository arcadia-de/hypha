#include "flatpak_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Flatpak){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewFlatpak() {
  return NewPackageManager(kFlatpakName, NULL, &kFlatpakConfig, NULL, NULL);
}
