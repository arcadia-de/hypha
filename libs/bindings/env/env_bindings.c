#include "env_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

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

LUA_FN(all) {
  const char delims[] = "=";

  lua_newtable(L);

  for (char** env = environ; *env != NULL; env++) {
    char* str = (*env);

    char* key = strtok(str, delims);
    char* value = strtok(NULL, delims);

    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
  }

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
