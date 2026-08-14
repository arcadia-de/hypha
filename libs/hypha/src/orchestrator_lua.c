#include <dlfcn.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "hypha/log.h"
#include "hypha/orchestrator.h"

lua_State* NewOrchestratorLuaState(Orchestrator* orc) {
  lua_State* L = luaL_newstate();
  if (!L)
    goto finished;

  luaL_openlibs(L);

  // LOG_DEBUG("setting lua registry value for the orchestrator");
  // lua_pushlightuserdata(L, orc);
  // lua_setfield(L, LUA_REGISTRYINDEX, CONTEXT_REGISTRY_KEY_ORCHESTRATOR);
  // LOG_DEBUG("done creating lua state");

finished:
  if (!L)
    LOG_ERROR("failed to creat new lua state");
  return L;
}

#define L OrchestratorGetLuaState(handle)

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
