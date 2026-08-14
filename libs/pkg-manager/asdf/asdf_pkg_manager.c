#include "asdf_pkg_manager.h"

#include "hypha/package_manager.h"

DEFINE_PACKAGE_MANAGER_CONFIG(Asdf){
    .install = NULL,
    .status = NULL,
    .uninstall = NULL,
};
PackageManager* NewAsdf() {
  return NewPackageManager(kAsdfName, NULL, &kAsdfConfig, NULL, NULL);
}
