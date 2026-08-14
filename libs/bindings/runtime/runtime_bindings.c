#include "runtime_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha/process.h"

LUA_FN(getVersion) {
  lua_pushstring(L, "0.0.0");
  return 1;
}

LUA_FN(exec) {
  // const char* bin;
  //
  // const char** args;
  // uint64_t num_args;
  //
  // const char** env_variables;
  // uint64_t num_env_variables;
  //
  // void* data;
  // ProcessLogFn out;
  // ProcessLogFn err;
  return 0;
}

LUA_FN(which) {
  char* bin_path = NULL;
  const char* bin = luaL_checkstring(L, 1);
  if (!ExecWhich(bin, &bin_path)) {
    luaL_error(L, "failed to exec `which %s`", bin);
    return 1;
  }

  lua_pushstring(L, bin_path);
  return 1;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(getVersion)
  BIND(exec)
  BIND(which)
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
#undef BIND
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_runtime, kFuncs);
