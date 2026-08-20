#include "host_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "hypha/process.h"

// --- Get all the info about the host
// ---@return hypha.host.HostInfo
// function M.info() end
//
// --- Execute /usr/bin/which for a specific bin
// ---@param bin string The binary to search for
// ---@return string
// function M.find(bin) end

LUA_FN(os) {
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

LUA_FN(hostname) {
  char hostname[HOST_NAME_MAX + 1];
  if (gethostname(hostname, HOST_NAME_MAX + 1) != 0)
    return luaL_error(L, "error: failed to get hostname");
  lua_pushstring(L, hostname);
  return 1;
}

LUA_FN(username) {
#if defined(_WIN32) || defined(_WIN64)
  lua_pushstring(L, getenv("USERNAME"));
#else
  lua_pushstring(L, getenv("USER"));
#endif
  return 1;
}

LUA_FN(kernel) {
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

LUA_FN(arch) {
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

#define OS_RELEASE_FILENAME "/etc/os-release"

LUA_FN(distro) {
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

LUA_FN(find) {
  return luaL_error(L, "`%s` is not implemented yet", __PRETTY_FUNCTION__);
}

LUA_FN(info) {
  return luaL_error(L, "`%s` is not implemented yet", __PRETTY_FUNCTION__);
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, &lua_##Name},
  BIND(os)
  BIND(username)
  BIND(hostname)
  BIND(kernel)
  BIND(arch)
  BIND(distro)
  BIND(has)
  BIND(info)
  BIND(find)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_host, kFuncs);
