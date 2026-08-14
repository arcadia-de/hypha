#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha.h"
#include "jsonnet_bindings.h"

static inline int LuaRenderJsonnet(lua_State* L) {
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
#define _BIND(Name, Func) {#Name, &Lua##Func}
  _BIND(render, RenderJsonnet),
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
#undef _BIND 
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_jsonnet, kFuncs);
