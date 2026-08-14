#include "manifest_bindings.h"

#include <stdlib.h>

#include "hypha.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "lua.h"

LUA_FN(file) {
  const int nargs = lua_gettop(L);
  if (nargs != 1)
    return luaL_error(L, "expected 1 args of type string or table");

  char* expanded = NULL;
  size_t expanded_len = 0;
  Expander expander;
  if (lua_isstring(L, 1)) {
    const char* p = luaL_checkstring(L, 1);

    if (!Expand(&expander, p, strlen(p), (char**)&expanded, &expanded_len)) {
      DLOG_WARN("failed to expand: %s", p);
      lua_pushstring(L, p);
    } else {
      lua_pushstring(L, expanded);
      free(expanded);
    }
  } else if (lua_istable(L, 1)) {
    const size_t len = lua_rawlen(L, 1);

    lua_newtable(L);
    const int new_table_idx = lua_gettop(L);
    for (size_t i = 1; i <= len; i++) {
      lua_rawgeti(L, 1, i);
      if (!lua_isstring(L, -1)) {
        DLOG_WARN("expected table value at %zu to be a string", i);
        lua_pop(L, 1);
        continue;
      }

      const char* s = lua_tostring(L, -1);
      if (!Expand(&expander, s, strlen(s), (char**)&expanded, &expanded_len)) {
        DLOG_WARN("failed to expand: %s", s);
        lua_pushstring(L, s);
      } else {
        lua_pop(L, 1);
        lua_pushstring(L, expanded);
        free(expanded);
      }

      lua_rawseti(L, new_table_idx, i);
    }
  }

  return 1;
}

LUA_FN(glob) {
  lua_pushfstring(L, "%s is not implemented", __PRETTY_FUNCTION__);
  return 1;
}

LUA_FN(dir) {
  lua_pushfstring(L, "%s is not implemented", __PRETTY_FUNCTION__);
  return 1;
}

LUA_FN(raw) {
  lua_pushfstring(L, "%s is not implemented", __PRETTY_FUNCTION__);
  return 1;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(file)
  BIND(glob)
  BIND(dir)
  BIND(raw)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_manifest, kFuncs);
