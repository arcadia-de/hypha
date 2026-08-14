#include "host_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "hypha/process.h"

LUA_FN(getOS) {
#if defined(_WIN32) || defined(_WIN64)
  lua_pushstring(L, "windows");
#elif defined(__APPLE__) || defined(__MACH__)
  lua_pushstring(L, "darwin");
#elif defined(__linux__)
  lua_pushstring(L, "linux");
#elif defined(__FreeBSD__)
  lua_pushstring(L, "freebsd");
#elif defined(__unix__) || defined(__unix)
  lua_pushstring(L, "unix");
#else
  lua_pushstring(L, "Unknown");
#endif
  return 1;
}

LUA_FN(isWindows) {
#if defined(_WIN32) || defined(_WIN64)
  lua_pushboolean(L, true);
#else
  lua_pushboolean(L, false);
#endif
  return 1;
}

LUA_FN(isFreeBSD) {
#if defined(__FreeBSD__)
  lua_pushboolean(L, true);
#else
  lua_pushboolean(L, false);
#endif
  return 1;
}

LUA_FN(isUnix) {
#if defined(__unix__) || defined(__unix)
  lua_pushboolean(L, true);
#else
  lua_pushboolean(L, false);
#endif
  return 1;
}

LUA_FN(isLinux) {
#if defined(__linux__)
  lua_pushboolean(L, true);
#else
  lua_pushboolean(L, false);
#endif
  return 1;
}

LUA_FN(isOSX) {
#if defined(__APPLE__) || defined(__MACH__)
  lua_pushboolean(L, true);
#else
  lua_pushboolean(L, false);
#endif
  return 1;
}

LUA_FN(getHostname) {
  char hostname[HOST_NAME_MAX + 1];
  if (gethostname(hostname, HOST_NAME_MAX + 1) != 0)
    return luaL_error(L, "error: failed to get hostname");
  lua_pushstring(L, hostname);
  return 1;
}

LUA_FN(getUsername) {
#if defined(_WIN32) || defined(_WIN64)
  lua_pushstring(L, getenv("USERNAME"));
#else
  lua_pushstring(L, getenv("USER"));
#endif
  return 1;
}

LUA_FN(getKernelInfo) {
  struct utsname kernel;
  if (uname(&kernel) != 0)
    return luaL_error(L, "error: failed to get kernel info");

  lua_newtable(L);

#define SET_FIELD(Name)           \
  lua_pushstring(L, kernel.Name); \
  lua_setfield(L, -2, #Name);

  SET_FIELD(version);
  SET_FIELD(release);
  SET_FIELD(sysname);
  SET_FIELD(machine);
  SET_FIELD(nodename);
#undef SET_FIELD
  return 1;
}

LUA_FN(getArch) {
#if defined(__x86_64__) || defined(_M_X64)
  lua_pushstring(L, "x86_64");
#elif defined(__i386__) || defined(_M_IX86)
  lua_pushstring(L, "x86");
#elif defined(__aarch64__) || defined(_M_ARM64)
  lua_pushstring(L, "arm64");
#elif defined(__arm__) || defined(_M_ARM)
  lua_pushstring(L, "arm");
#else
  lua_pushstring(L, "unknown");
#endif
  return 1;
}

LUA_FN(getKernelVersion) {
  struct utsname kernel;
  if (uname(&kernel) != 0)
    return luaL_error(L, "error: failed to get kernel version");
  lua_pushstring(L, kernel.version);
  return 1;
}

#define OS_RELEASE_FILENAME "/etc/os-release"

LUA_FN(getDistro) {
  FILE* fp = fopen(OS_RELEASE_FILENAME, "r");
  if (fp == NULL)
    return luaL_error(L, "error: unable to open %s", OS_RELEASE_FILENAME);

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    // Look for PRETTY_NAME or NAME keys
    if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
      // Extract the value inside quotes
      char* start = strchr(line, '"');
      char* end = strrchr(line, '"');
      if (start && end && start != end) {
        *end = '\0';
        lua_pushstring(L, start + 1);
        goto finished;
      }
    }
  }

  lua_pushstring(L, "unknown");
finished:
  fclose(fp);
  return 1;
}

LUA_FN(has) {
  const char* pkg = luaL_checkstring(L, 1);

  char* result = NULL;
  if (!ExecWhich(pkg, &result))
    return false;

  lua_pushboolean(L, result != NULL);
  return 1;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, &lua_##Name},
  BIND(getOS)
  BIND(getUsername)
  BIND(getHostname)
  BIND(getKernelInfo)
  BIND(getKernelVersion)
  BIND(getArch)
  BIND(getDistro)
  BIND(isOSX)
  BIND(isLinux)
  BIND(isWindows)
  BIND(isFreeBSD)
  BIND(isUnix)
  BIND(has)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_host, kFuncs);
