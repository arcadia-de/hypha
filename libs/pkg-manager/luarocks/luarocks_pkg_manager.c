#include "luarocks_pkg_manager.h"

#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "hypha/process.h"

PACKAGE_MANAGER_INSTALL_FN(LuaRocks) {
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  static const int kNumberOfArgs = 4;
  const char* args[kNumberOfArgs];
  args[0] = "install";
  args[1] = "--tree";
  args[2] = (char*)data;
  args[3] = pkg;

  const int code = ExecPackageManager(mgr, args, kNumberOfArgs, false);
  if (code != 0) {
    status = kPackageUninstalled;
    goto finished;
  }

  status = kPackageInstalled;
finished:
  return status;
}

PACKAGE_MANAGER_UNINSTALL_FN(LuaRocks) {
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  //   const char* args[2];
  //   args[0] = "-R";
  //   args[1] = pkg;
  //
  //   const int code = ExecPackageManager(mgr, args, 2, false);
  //   if (code != 0) {
  //     status = kPackageUninstalled;
  //     goto finished;
  //   }
  //
  //   status = kPackageInstalled;
  // finished:
  return status;
}

PACKAGE_MANAGER_STATUS_FN(LuaRocks) {
  ASSERT(pkg);
  PackageStatus status = kPackageSkipped;

  //   const char* args[2];
  //   args[0] = "-Q";
  //   args[1] = pkg;
  //
  //   const int code = ExecPackageManager(mgr, args, 2, false);
  //   if (code != 0) {
  //     status = kPackageUninstalled;
  //     goto finished;
  //   }
  //
  //   status = kPackageInstalled;
  // finished:
  return status;
}

DEFINE_PACKAGE_MANAGER_CONFIG(Luarocks){
    .install = &LuaRocksInstall,
    .status = &LuaRocksStatus,
    .uninstall = &LuaRocksUninstall,
};
PackageManager* NewLuarocks(const char* dir) {
  return NewPackageManager(kLuarocksName, NULL, &kLuarocksConfig, strdup(dir), free);
}
