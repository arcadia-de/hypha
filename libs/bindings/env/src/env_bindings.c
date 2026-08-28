#include "hypha/env_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/env.h"

LUA_FN(get) {
  const char* name = luaL_checkstring(L, 1);
  char* value = getenv(name);
  if (value) {
    lua_pushstring(L, value);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

LUA_FN(has) {
  const char* name = luaL_checkstring(L, 1);
  char* value = getenv(name);
  lua_pushboolean(L, value != NULL);
  return 1;
}

extern char** environ;

static inline bool AppendEnvVar(uint64_t idx, const char* k, const char* v, void* data) {
#define L ((lua_State*)data)
  lua_pushstring(L, v);
  lua_setfield(L, -2, k);
#undef L
  return true;
}

LUA_FN(all) {
  lua_newtable(L);
  VisitAllEnvVars(&AppendEnvVar, L);
  return 1;
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
