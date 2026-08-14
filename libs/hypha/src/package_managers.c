#include "package_managers.h"

#include <stdlib.h>

#include "apt_pkg_manager.h"
#include "asdf_pkg_manager.h"
#include "brew_pkg_manager.h"
#include "cargo_pkg_manager.h"
#include "dnf_pkg_manager.h"
#include "eget_pkg_manager.h"
#include "flatpak_pkg_manager.h"
#include "go_pkg_manager.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"
#include "luarocks_pkg_manager.h"
#include "pacman_pkg_manager.h"
#include "paru_pkg_manager.h"
#include "yay_pkg_manager.h"

void InitPackageManagers(const char* luarocks_dir) {
  PackageManager* Apt = NewApt();
  DLOG_WARN_IF(!Apt, "failed to create Apt package manager");

  PackageManager* Asdf = NewAsdf();
  DLOG_WARN_IF(!Asdf, "failed to create Asdf package manager");

  PackageManager* Pacman = NewPacman();
  DLOG_WARN_IF(!Pacman, "failed to create Pacman package manager");

  PackageManager* Yay = NewYay();
  DLOG_WARN_IF(!Yay, "failed to create Yay package manager");

  PackageManager* Paru = NewParu();
  DLOG_WARN_IF(!Paru, "failed to create Paru package manager");

  PackageManager* Luarocks = NewLuarocks(luarocks_dir);
  DLOG_WARN_IF(!Luarocks, "failed to create Luarocks package manager");

  PackageManager* brew = NewBrew();
  DLOG_WARN_IF(!brew, "failed to create Brew package manager");

  PackageManager* Cargo = NewCargo();
  DLOG_WARN_IF(!Cargo, "failed to create Cargo package manager");

  PackageManager* Dnf = NewDnf();
  DLOG_WARN_IF(!Dnf, "failed to create Dnf package manager");

  PackageManager* Eget = NewEget();
  DLOG_WARN_IF(!Eget, "failed to create Eget package manager");

  PackageManager* Flatpak = NewFlatpak();
  DLOG_WARN_IF(!Flatpak, "failed to create Flatpak package manager");

  PackageManager* Go = NewGo();
  DLOG_WARN_IF(!Go, "failed to create Go package manager");
}
