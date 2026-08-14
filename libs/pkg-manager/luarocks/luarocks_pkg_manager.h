#ifndef HYPHA_LUAROCKS_PKG_MANAGER_H
#define HYPHA_LUAROCKS_PKG_MANAGER_H

#include "hypha/package_manager.h"

PackageManager* NewLuarocks(const char* dir);

#endif  // HYPHA_LUAROCKS_PKG_MANAGER_H
