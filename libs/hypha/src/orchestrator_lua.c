#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "hypha/env_bindings.h"
#include "hypha/events_bindings.h"
#include "hypha/host_bindings.h"
#include "hypha/log.h"
#include "hypha/log_bindings.h"
#include "hypha/orchestrator.h"
#include "hypha/packages_bindings.h"
#include "hypha/runtime_bindings.h"
#include "hypha/sources_bindings.h"
#include "orc.h"

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

  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");

#define BIND(Name)                            \
  lua_pushcfunction(L, luaopen_hypha_##Name); \
  lua_setfield(L, -2, "hypha." #Name);

  BIND(env);
  BIND(events);
  BIND(host);
  BIND(log);
  BIND(packages);
  BIND(runtime);
  BIND(sources);
#undef BIND

  lua_pop(L, 2);
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
