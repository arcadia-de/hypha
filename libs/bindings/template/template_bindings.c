#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha.h"
#include "template_bindings.h"

typedef char* (*RenderTemplateFunc)(char*, char*, bool);

static inline int LuaRenderTemplate(lua_State* L) {
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

static const struct luaL_Reg kFuncs[] = {
    {"render", &LuaRenderTemplate},
    {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};

DEFINE_LUA_BINDINGS(hypha_template, kFuncs);
