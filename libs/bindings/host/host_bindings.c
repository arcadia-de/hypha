#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "host_bindings.h"

static inline const char* GetOSName() {
#if defined(_WIN32) || defined(_WIN64)
  return "windows";
#elif defined(__APPLE__) || defined(__MACH__)
  return "darwin";
#elif defined(__linux__)
  return "linux";
#elif defined(__FreeBSD__)
  return "freebsd";
#elif defined(__unix__) || defined(__unix)
  return "unix";
#else
  return "Unknown";
#endif
}

static inline int GetOperatingSystemName(lua_State* L) {
  lua_pushstring(L, GetOSName());
  return 1;
}

static inline const char* GetUserNameEnv() {
#if defined(_WIN32) || defined(_WIN64)
  return getenv("USERNAME");
#else
  return getenv("USER");
#endif
}

static inline int GetUserName(lua_State* L) {
  lua_pushstring(L, GetUserNameEnv());
  return 1;
}

static const struct luaL_Reg kFuncs[] = {
    {"getOperatingSystemName", &GetOperatingSystemName},
    {"getUserName", &GetUserName},
    {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};

DEFINE_LUA_BINDINGS(hypha_host, kFuncs);
