#ifndef HYPHA_LUA_CONTROLLER_H
#define HYPHA_LUA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <lua.h>

#include "hypha/controller.h"

typedef struct {
  lua_State* L;
  int init_ref;
  int deinit_ref;
  int observe_ref;
  int plan_ref;
  int apply_ref;
  int destroy_ref;
  int validate_ref;
  int diff_ref;
  int status_ref;
  int rollback_ref;
  int normalize_ref;
} LuaController;

Controller* NewLuaController(lua_State* L, const char* kind, const int tbl_index);
void FreeLuaController(LuaController* ctrl);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  //  HYPHA_LUA_CONTROLLER_H
