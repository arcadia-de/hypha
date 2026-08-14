#include "packages_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"

LUA_FN(PackageManagerReadOnly) {
  const char* key = luaL_checkstring(L, 2);
  return luaL_error(L, "error: field '%s' is read-only for PackageManagers", key);
}

#define UNWRAP_PACKAGE_MANAGER(L, Index, Name)                   \
  luaL_checktype(L, Index, LUA_TTABLE);                          \
  lua_getfield(L, Index, "_handle");                             \
  PackageManager* Name = (PackageManager*)lua_touserdata(L, -1); \
  lua_pop(L, Index);                                             \
  if (!mgr)                                                      \
    return luaL_error(L, "PackageManager _handle was null");

LUA_FN(PackageManagerUninstall) {
  UNWRAP_PACKAGE_MANAGER(L, 1, mgr);
  const char* pkg = luaL_checkstring(L, 2);
  PackageStatus status = PackageManagerUninstall(mgr, pkg);
  lua_pushstring(L, PackageStatusName(status));
  return 1;
}

LUA_FN(PackageManagerInstall) {
  UNWRAP_PACKAGE_MANAGER(L, 1, mgr);
  const char* pkg = luaL_checkstring(L, 2);
  LOG_INFO("installing %s", pkg);
  PackageStatus status = PackageManagerInstall(mgr, pkg);
  lua_pushstring(L, PackageStatusName(status));
  return 1;
}

LUA_FN(PackageManagerStatus) {
  UNWRAP_PACKAGE_MANAGER(L, 1, mgr);
  const char* pkg = luaL_checkstring(L, 2);
  PackageStatus status = PackageManagerStatus(mgr, pkg);
  lua_pushstring(L, PackageStatusName(status));
  return 1;
}

static inline void CreatePackageManagerTable(lua_State* L, PackageManager* pkg) {
  lua_newtable(L);

  lua_pushstring(L, GetPackageManagerName(pkg));
  lua_setfield(L, -2, "name");

  lua_pushstring(L, GetPackageManagerPath(pkg));
  lua_setfield(L, -2, "path");

  lua_pushlightuserdata(L, pkg);
  lua_setfield(L, -2, "_handle");

  if (luaL_newmetatable(L, "PackageManager")) {
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, &lua_PackageManagerReadOnly);
    lua_setfield(L, -2, "__newindex");

    lua_pushcfunction(L, &lua_PackageManagerStatus);
    lua_setfield(L, -2, "status");

    lua_pushcfunction(L, &lua_PackageManagerInstall);
    lua_setfield(L, -2, "install");

    lua_pushcfunction(L, &lua_PackageManagerUninstall);
    lua_setfield(L, -2, "uninstall");
  }

  lua_setmetatable(L, -2);
}

LUA_FN(getAllPackageManagers) {
  lua_newtable(L);
  const int result_idx = lua_gettop(L);

  for (uint64_t i = 0; i < GetNumberOfPackageManagers(); i++) {
    PackageManager* m = GetPackageManagerAt(i);
    ASSERT(m);
    lua_Unsigned len = lua_rawlen(L, result_idx);
    CreatePackageManagerTable(L, m);
    lua_rawseti(L, result_idx, (int)len + 1);
  }

  return 1;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(getAllPackageManagers)
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
#undef BIND
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_packages, kFuncs);
