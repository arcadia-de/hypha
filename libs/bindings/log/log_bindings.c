#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha/log.h"
#include "log_bindings.h"

#define DEFINE_LUA_LOG_FUNC(Level)                \
  static inline int LuaLog##Level(lua_State* L) { \
    LOG_##Level("%s", luaL_checkstring(L, 1));    \
    return 0;                                     \
  }

DEFINE_LUA_LOG_FUNC(INFO);
DEFINE_LUA_LOG_FUNC(SUCCESS);
DEFINE_LUA_LOG_FUNC(DEBUG);
DEFINE_LUA_LOG_FUNC(WARN);
DEFINE_LUA_LOG_FUNC(ERROR);

static inline int LuaLogFATAL(lua_State* L) {
  // TODO(@s0cks): need to properly shutdown system before exiting
  LOG_ERROR("%s", luaL_checkstring(L, 1));
  return 0;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
    {"info", LuaLogINFO},
    {"success", LuaLogSUCCESS},
    {"debug", LuaLogDEBUG},
    {"warn", LuaLogWARN},
    {"error", LuaLogERROR},
    {"fatal", LuaLogFATAL},
    {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_log, kFuncs);
