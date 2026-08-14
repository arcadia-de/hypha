#include "env_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

LUA_FN(get) {
  return luaL_error(L, "error: '%s' not implemented", __PRETTY_FUNCTION__);
}

LUA_FN(has) {
  return luaL_error(L, "error: '%s' not implemented", __PRETTY_FUNCTION__);
}

LUA_FN(all) {
  return luaL_error(L, "error: '%s' not implemented", __PRETTY_FUNCTION__);
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, &lua_##Name},
  BIND(get)
  BIND(has)
  BIND(all)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_env, kFuncs);
