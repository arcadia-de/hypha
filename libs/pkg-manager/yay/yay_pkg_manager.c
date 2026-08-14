#include "yay_pkg_manager.h"

#include <stdlib.h>
#include <string.h>

#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"

PACKAGE_MANAGER_STATUS_FN(Yay) {
  PackageStatus status = kPackageSkipped;
  if (!pkg) {
    LOG_ERROR("package name is null");
    goto finished;
  }

  Process proc;
  memset(&proc, 0, sizeof(Process));
  proc.bin = GetPackageManagerPath(mgr);

  const char* args[2];
  args[0] = "-Q";
  args[1] = pkg;

  proc.num_args = 2;
  proc.args = args;

  const int code = ExecProcess(&proc);
  if (code != 0) {
    status = kPackageUninstalled;
    goto finished;
  }

  status = kPackageInstalled;
finished:
  return status;
}

DEFINE_PACKAGE_MANAGER_CONFIG(Yay){
    .status = &YayStatus,
};
PackageManager* NewYay() {
  return NewPackageManager(kYayName, NULL, &kYayConfig, NULL, NULL);
}
