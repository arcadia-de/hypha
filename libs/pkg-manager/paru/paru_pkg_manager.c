#include "paru_pkg_manager.h"

#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"

PACKAGE_MANAGER_INSTALL_FN(Paru) {
  ASSERT(mgr);
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  const char* args[2];
  args[0] = "-S";
  args[1] = pkg;

  const int code = ExecPackageManager(mgr, args, 2, false);
  if (code != 0) {
    status = kPackageError;
    goto finished;
  }

  status = kPackageInstalled;
finished:
  return status;
}

PACKAGE_MANAGER_UNINSTALL_FN(Paru) {
  ASSERT(mgr);
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  const char* args[2];
  args[0] = "-R";
  args[1] = pkg;

  const int code = ExecPackageManager(mgr, args, 2, false);
  if (code != 0) {
    status = kPackageError;
    goto finished;
  }

  status = kPackageUninstalled;
finished:
  return status;
}

PACKAGE_MANAGER_STATUS_FN(Paru) {
  ASSERT(mgr);
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

DEFINE_PACKAGE_MANAGER_CONFIG(Paru){
    .install = &ParuInstall,
    .uninstall = &ParuUninstall,
    .status = &ParuStatus,
};
PackageManager* NewParu() {
  return NewPackageManager(kParuName, NULL, &kParuConfig, NULL, NULL);
}
