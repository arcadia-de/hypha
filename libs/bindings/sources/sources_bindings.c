#include "sources_bindings.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/expander.h"
#include "hypha/glob.h"
#include "hypha/log.h"
#include "lua.h"

LUA_FN(file) {
  const int nargs = lua_gettop(L);
  if (nargs != 1)
    return luaL_error(L, "expected 1 args of type string or table");

  lua_newtable(L);
  const int new_table_idx = lua_gettop(L);
  char* expanded = NULL;
  size_t expanded_len = 0;
  Expander expander;

  if (lua_isstring(L, 1)) {
    const char* p = luaL_checkstring(L, 1);
    lua_pushnumber(L, kDiscoveredPath);
    lua_setfield(L, new_table_idx, "kind");
    if (!Expand(&expander, p, strlen(p), (char**)&expanded, &expanded_len)) {
      DLOG_WARN("failed to expand: %s", p);
      lua_pushstring(L, p);
    } else {
      lua_pushstring(L, expanded);
      free(expanded);
    }
    lua_setfield(L, new_table_idx, "value");
  } else if (lua_istable(L, 1)) {
    const size_t len = lua_rawlen(L, 1);
    int file_row_counter = 1;  // FIXED: Prevent nil index gaps when values are skipped

    for (size_t i = 1; i <= len; i++) {
      lua_rawgeti(L, 1, i);
      if (!lua_isstring(L, -1)) {
        DLOG_WARN("expected table value at %zu to be a string", i);
        lua_pop(L, 1);
        continue;
      }
      const char* s = lua_tostring(L, -1);
      lua_newtable(L);
      const int new_tbl_idx = lua_gettop(L);
      lua_pushnumber(L, kDiscoveredPath);
      lua_setfield(L, new_tbl_idx, "kind");
      if (!Expand(&expander, s, strlen(s), (char**)&expanded, &expanded_len)) {
        DLOG_WARN("failed to expand: %s", s);
        lua_pushstring(L, s);
      } else {
        lua_pushstring(L, expanded);
        free(expanded);
      }
      lua_setfield(L, new_tbl_idx, "value");

      // Force tight sequential keys to safeguard the discovery.c loop length calculations
      lua_rawseti(L, new_table_idx, file_row_counter);
      file_row_counter++;
      lua_pop(L, 1);
    }
  }
  return 1;
}

LUA_FN(glob) {
  const char* raw_dir = luaL_checkstring(L, 1);
  bool recursive = false;
  char* pattern = NULL;

  if (lua_istable(L, 2)) {
    {
      const int type = lua_getfield(L, 2, "pattern");
      if (type == LUA_TSTRING)
        pattern = strdup(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
    {
      const int type = lua_getfield(L, 2, "recursive");
      if (type == LUA_TBOOLEAN)
        recursive = lua_toboolean(L, -1);
      lua_pop(L, 1);
    }
  }

  Glob glob;
  memset(&glob, 0, sizeof(Glob));
  InitGlob(&glob, 32);

  char* new_dir = NULL;
  size_t new_dir_len = 0;
  Expander expander;

  if (!Expand(&expander, raw_dir, strlen(raw_dir), &new_dir, &new_dir_len)) {
    GlobFiles(raw_dir, pattern, &glob, recursive);
  } else {
    GlobFiles(new_dir, pattern, &glob, recursive);
  }

  if (!lua_checkstack(L, (int)glob.paths_len * 3 + 4)) {
    if (new_dir)
      free(new_dir);
    if (pattern)
      free(pattern);
    ClearGlob(&glob);
    return luaL_error(L, "Lua stack overflow expanding glob array context");
  }

  int num_rows = 1;
  lua_newtable(L);
  const int res_tbl_idx = lua_gettop(L);

  for (size_t i = 0; i < glob.paths_len; i++) {
    if (!glob.paths || !glob.paths[i])
      continue;

    lua_newtable(L);

    lua_pushnumber(L, kDiscoveredPath);
    lua_setfield(L, -2, "kind");

    // FIXED: Force a deep byte-copy allocation inside the Lua VM
    // to insulate the data from ClearGlob's C-heap teardown
    lua_pushlstring(L, glob.paths[i], strlen(glob.paths[i]));
    lua_setfield(L, -2, "value");

    lua_rawseti(L, res_tbl_idx, num_rows);
    num_rows++;
  }

  if (new_dir)
    free(new_dir);
  if (pattern)
    free(pattern);

  // Safe to clear now—Lua owns its completely independent string copies
  ClearGlob(&glob);

  lua_settop(L, res_tbl_idx);
  return 1;
}

static inline void StripWhitespace(char* str) {
  int i = 0;
  int j = 0;
  while (str[i] != '\0') {
    const unsigned char c = (unsigned char)str[i];
    if (!isspace(c) && c != '\t' && c != '\r')
      str[j++] = str[i];

    i++;
  }
  str[j] = '\0';
}

LUA_FN(dir) {
  return 0;
}

LUA_FN(raw) {
  const char* tpl = luaL_checkstring(L, 1);

  char* new_tpl = strdup(tpl);
  StripWhitespace(new_tpl);

  lua_newtable(L);
  lua_pushnumber(L, kDiscoveredRaw);
  lua_setfield(L, -2, "kind");

  lua_pushstring(L, new_tpl);
  lua_setfield(L, -2, "value");

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

DEFINE_LUA_BINDINGS(hypha_sources, kFuncs);
