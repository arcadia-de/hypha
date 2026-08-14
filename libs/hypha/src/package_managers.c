#include "package_managers.h"

#include <stdlib.h>

#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "pacman_pkg_manager.h"
#include "paru_pkg_manager.h"
#include "yay_pkg_manager.h"

void InitPackageManagers() {
  PackageManager* pacman = CreatePacmanPackageManager();
  DLOG_INFO_IF(!pacman, "failed to create pacman package manager");

  PackageManager* yay = CreateYayPackageManager();
  DLOG_INFO_IF(!yay, "failed to create yay package manager");

  PackageManager* paru = CreateParuPackageManager();
  DLOG_INFO_IF(!paru, "failed to create paru package manager");
}
