#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "hypha/log.h"
#include "hypha/orchestrator.h"

static inline void InitRegData(lua_State* L, Orchestrator* orc) {
  lua_pushlightuserdata(L, orc);
  lua_setfield(L, LUA_REGISTRYINDEX, LUA_REGISTRY_ORC_KEY);

  lua_pushlightuserdata(L, GetOrcEventBus(orc));
  lua_setfield(L, LUA_REGISTRYINDEX, LUA_REGISTRY_EVENTS_KEY);
}

static inline void OpenLuaLibs(lua_State* L) {
  luaL_requiref(L, "_G", luaopen_base, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1);
  lua_pop(L, 1);
}

lua_State* NewOrchestratorLuaState(Orchestrator* orc) {
  lua_State* L = luaL_newstate();
  if (!L)
    goto finished;

  OpenLuaLibs(L);
  InitRegData(L, orc);

finished:
  if (!L)
    LOG_ERROR("failed to creat new lua state");
  return L;
}

#define L GetOrcLuaState(handle)

bool OrchestratorEvalExpr(OrchestratorHandle handle, const char* expr, char** err) {
  ASSERT(handle);
  ASSERT(expr);
  int result = luaL_dostring(L, expr);
  if (result != LUA_OK) {
    (*err) = strdup(lua_tostring(L, -1));
    return false;
  }

  (*err) = NULL;
  return true;
}

bool OrchestratorEvalFile(OrchestratorHandle handle, const char* filename, char** err) {
  ASSERT(handle);
  ASSERT(filename);
  int result = luaL_dofile(L, filename);
  if (result != LUA_OK) {
    (*err) = strdup(lua_tostring(L, -1));
    return false;
  }

  (*err) = NULL;
  return true;
}

#undef L
