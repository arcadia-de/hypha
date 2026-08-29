#include "hypha/sources_bindings.h"

#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "hypha.h"
#include "hypha/discovered_manifest.h"
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
    int file_row_counter = 1;

    for (size_t i = 1; i <= len; i++) {
      lua_rawgeti(L, 1, (lua_Integer)i);
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

    lua_pushlstring(L, glob.paths[i], strlen(glob.paths[i]));
    lua_setfield(L, -2, "value");

    lua_rawseti(L, res_tbl_idx, num_rows);
    num_rows++;
  }

  if (new_dir)
    free(new_dir);
  if (pattern)
    free(pattern);

  ClearGlob(&glob);

  lua_settop(L, res_tbl_idx);
  return 1;
}

typedef bool (*WalkContextCallbackFn)(uint32_t idx, const char* path, void* data);

typedef struct {
  WalkContextCallbackFn fn;
  void* data;

  char** patterns;
  size_t patterns_len;

  size_t num_files;
} WalkContext;

typedef struct {
  lua_State* L;
  int result_index;
} LuaWalkData;

static inline void WalkDir(const char* base_path, WalkContext* ctx) {
  DIR* dir = opendir(base_path);
  if (!dir)
    return;

  char path[PATH_MAX];
  struct dirent* dp = NULL;
  while ((dp = readdir(dir)) != NULL) {
    if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
      snprintf(path, sizeof(path), "%s/%s", base_path, dp->d_name);

      struct stat statbuf;
      if (stat(path, &statbuf) == 0) {
        if (S_ISDIR(statbuf.st_mode)) {
          WalkDir(path, ctx);
        } else if (S_ISREG(statbuf.st_mode)) {
          for (size_t i = 0; i < ctx->patterns_len; i++) {
            const char* pattern = ctx->patterns[i];
            if (fnmatch(pattern, dp->d_name, 0) == 0) {
              if (ctx->fn) {
                if (!ctx->fn(ctx->num_files, path, ctx->data))
                  goto finished;
                ctx->num_files++;
              }
            }
          }
        }
      }
    }
  }
finished:
  closedir(dir);
}

static inline bool OnDefaultSourceFound(uint32_t idx, const char* p, void* data) {
#define L ((LuaWalkData*)data)->L
  lua_newtable(L);
  const int new_tbl_idx = lua_gettop(L);
  lua_pushnumber(L, kDiscoveredPath);
  lua_setfield(L, new_tbl_idx, "kind");

  lua_pushstring(L, p);
  lua_setfield(L, new_tbl_idx, "value");

  lua_rawseti(L, -2, idx + 1);
#undef L
  return true;
}

LUA_FN(getDefaultSources) {
  WalkContext ctx;
  ctx.patterns = (char**)malloc(sizeof(char*) * 3);
  ctx.patterns[0] = "*.jsonnet";
  ctx.patterns[1] = "*.json";
  ctx.patterns[2] = "*.yaml";
  ctx.patterns_len = 3;
  ctx.fn = &OnDefaultSourceFound;
  ctx.num_files = 0;

  lua_newtable(L);
  int result_index = lua_gettop(L);
  LuaWalkData data = {
      .L = L,
      .result_index = result_index,
  };
  ctx.data = &data;
  WalkDir("/home/tazz/.config/hypha", &ctx);
  return 1;
}

LUA_FN(dir) {
  return 0;
}

LUA_FN(raw) {
  const int nargs = lua_gettop(L);
  const char* tpl = luaL_checkstring(L, 1);

  DiscoveredManifestKind kind = kDiscoveredRawJsonnet;
  if (nargs >= 2) {
    const char* raw_kind = luaL_checkstring(L, 2);
    if (strcasecmp(raw_kind, "jsonnet") == 0) {
      kind = kDiscoveredRawJsonnet;
    } else if (strcasecmp(raw_kind, "json") == 0) {
      kind = kDiscoveredRawJson;
    } else if (strcasecmp(raw_kind, "yaml") == 0) {
      kind = kDiscoveredRawYaml;
    } else {
      return luaL_error(L, "invalid raw manifest kind: %s", raw_kind);
    }
  }

  lua_newtable(L);
  lua_pushnumber(L, kind);
  lua_setfield(L, -2, "kind");

  lua_pushstring(L, tpl);
  lua_setfield(L, -2, "value");
  return 1;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(file)
  BIND(glob)
  BIND(dir)
  BIND(getDefaultSources)
  BIND(raw)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_sources, kFuncs);
