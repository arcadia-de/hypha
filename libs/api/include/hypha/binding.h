#ifndef HYPHA_BINDING_H
#define HYPHA_BINDING_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define DECLARE_LUA_BINDINGS(Name) int luaopen_##Name(lua_State*);

#define DEFINE_LUA_BINDINGS(Name, Funcs)    \
  extern int luaopen_##Name(lua_State* L) { \
    luaL_newlib(L, (Funcs));                \
    return 1;                               \
  }

#define LUA_FN(Name) static inline int lua_##Name(lua_State* L)

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_BINDING_H
