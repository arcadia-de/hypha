#include "pacman_pkg_manager.h"

#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"

PACKAGE_MANAGER_INSTALL_FN(Pacman) {
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  static const int kNumberOfArgs = 3;
  const char* args[kNumberOfArgs];
  args[0] = "-S";
  args[1] = "--noconfirm";
  args[2] = pkg;

  LOG_INFO("installing: %s", pkg);

  const int code = ExecPackageManager(mgr, args, kNumberOfArgs, true);
  if (code != 0) {
    status = kPackageUninstalled;
    goto finished;
  }

  status = kPackageInstalled;
finished:
  return status;
}

PACKAGE_MANAGER_UNINSTALL_FN(Pacman) {
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  const char* args[2];
  args[0] = "-R";
  args[1] = pkg;

  const int code = ExecPackageManager(mgr, args, 2, false);
  if (code != 0) {
    status = kPackageUninstalled;
    goto finished;
  }

  status = kPackageInstalled;
finished:
  return status;
}

PACKAGE_MANAGER_STATUS_FN(Pacman) {
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  const char* args[2];
  args[0] = "-Q";
  args[1] = pkg;

  const int code = ExecPackageManager(mgr, args, 2, false);
  if (code != 0) {
    status = kPackageUninstalled;
    goto finished;
  }

  status = kPackageInstalled;
finished:
  return status;
}

DEFINE_PACKAGE_MANAGER_CONFIG(Pacman){
    .install = &PacmanInstall,
    .status = &PacmanStatus,
    .uninstall = &PacmanUninstall,
};
PackageManager* NewPacman() {
  return NewPackageManager(kPacmanName, NULL, &kPacmanConfig, NULL, NULL);
}
