#include "runtime_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha/expander.h"
#include "hypha/lua_controller.h"
#include "hypha/process.h"

LUA_FN(getVersion) {
  lua_pushstring(L, "0.0.0");
  return 1;
}

LUA_FN(createController) {
  const char* kind = luaL_checkstring(L, 1);
  Controller* ctrl = NewLuaController(L, kind, 2);
  if (!ctrl)
    return luaL_error(L, "failed to create new lua controller for kind: %s", kind);
  ControllerInit(ctrl);
  return 0;
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

LUA_FN(expand) {
  const char* value = luaL_checkstring(L, 1);

  char* result = NULL;
  size_t result_len = 0;
  Expander expander;
  if (!ExpandStr(&expander, value, &result, &result_len))
    return luaL_error(L, "error: failed to expand: %s", value);

  lua_pushstring(L, result);
  return 1;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(getVersion)
  BIND(exec)
  BIND(which)
  BIND(expand)
  BIND(createController)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_runtime, kFuncs);
