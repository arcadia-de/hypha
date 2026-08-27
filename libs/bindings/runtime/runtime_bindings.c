#include "runtime_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha/expander.h"
#include "hypha/label.h"
#include "hypha/log.h"
#include "hypha/process.h"

static inline int lua_getVersion(lua_State* L) {
  lua_pushstring(L, "0.0.0");
  return 1;
}

LUA_FN(addDefaultLabels) {
  const int type = lua_type(L, 1);

  switch (type) {
    case LUA_TSTRING: {
      Label label;
      strncpy(label, luaL_checkstring(L, 1), HYPHA_LABEL_MAX_SIZE);
      DLOG_INFO("appending default label: %s", label);
      AppendDefaultLabel(label);
      return 0;
    }
    case LUA_TTABLE: {
      size_t len = lua_rawlen(L, 1);
      Label* labels = (Label*)malloc(sizeof(Label) * len);
      for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, 1, (lua_Integer)i);
        strncpy(labels[i - 1], luaL_checkstring(L, -1), HYPHA_LABEL_MAX_SIZE);
        DLOG_INFO("appending default label: %s", labels[i - 1]);
      }
      AppendDefaultLabels(labels, len);
      return 0;
    }
    default:
      return luaL_error(L, "invalid lua type: %s", lua_typename(L, type));
  }
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

LUA_FN(renderTemplate) {
  const int num_args = lua_gettop(L);

  if (num_args < 1)
    return luaL_error(L, "at least one argument is required");

  const char* tpl = luaL_checkstring(L, 1);

  bool yaml = false;
  const char* data = NULL;
  if (num_args >= 2) {
    if (lua_isstring(L, 2))
      data = lua_tostring(L, 2);
  }

  if (num_args >= 3) {
    if (lua_isboolean(L, 3))
      yaml = lua_toboolean(L, 3);
  }

  if (data == NULL)
    data = "{}";

  char* result = RenderTemplate((char*)tpl, (char*)data, yaml);
  if (result) {
    lua_pushstring(L, result);
    return 1;
  }

  luaL_error(L, "failed to render template");
  return 0;
}

LUA_FN(renderJsonnet) {
  const int num_args = lua_gettop(L);

  if (num_args < 1)
    return luaL_error(L, "at least one argument is required");

  const char* name = luaL_checkstring(L, 1);

  const char* tpl = NULL;
  if (num_args >= 2) {
    if (lua_isstring(L, 2))
      tpl = lua_tostring(L, 2);
  }

  char* result = RenderJsonnet((char*)name, (char*)tpl);
  if (result) {
    lua_pushstring(L, result);
    return 1;
  }

  luaL_error(L, "failed to render jsonnet");
  return 0;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
  { "version", lua_getVersion },
#define BIND(Name) {#Name, lua_##Name},
  BIND(which)
  BIND(expand)
  BIND(renderTemplate)
  BIND(renderJsonnet)
  BIND(addDefaultLabels)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on
DEFINE_LUA_BINDINGS(hypha_runtime, kFuncs);
