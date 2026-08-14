#include "log_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha/log.h"

#define DEFINE_LUA_LOG_FUNC(Name, Level)       \
  LUA_FN(log_##Name) {                         \
    LOG_##Level("%s", luaL_checkstring(L, 1)); \
    return 0;                                  \
  }

DEFINE_LUA_LOG_FUNC(info, INFO);
DEFINE_LUA_LOG_FUNC(success, SUCCESS);
DEFINE_LUA_LOG_FUNC(debug, DEBUG);
DEFINE_LUA_LOG_FUNC(warn, WARN);
DEFINE_LUA_LOG_FUNC(error, ERROR);

LUA_FN(log_fatal) {
  // TODO(@s0cks): need to properly shutdown system before exiting
  LOG_ERROR("%s", luaL_checkstring(L, 1));
  return 0;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_log_##Name},
  BIND(info)
  BIND(success)
  BIND(debug)
  BIND(warn)
  BIND(error)
  BIND(fatal)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_log, kFuncs);
