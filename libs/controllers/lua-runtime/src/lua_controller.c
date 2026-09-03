#include "hypha/lua_controller.h"

#include <lauxlib.h>
#include <lua.h>
#include <memory.h>
#include <stdlib.h>

#include "hypha/controller.h"
#include "hypha/log.h"
#include "hypha/resource_kind.h"

static inline bool BindCallbackField(lua_State* L, int tbl_index, const char* field_name, int* ref) {
  (*ref) = LUA_NOREF;

  if (!lua_checkstack(L, 2))
    return false;

  const int type = lua_getfield(L, tbl_index, field_name);
  if (type == LUA_TFUNCTION) {
    (*ref) = luaL_ref(L, LUA_REGISTRYINDEX);
    return true;
  }

  lua_pop(L, 1);
  if (type != LUA_TNIL)
    luaL_error(L, "expected 'init' field to be a function");
  return type == LUA_TNIL;
}

DEFINE_CONTROLLER_INIT_FN(Lua) {
  LuaController* lctrl = (LuaController*)data;
  if (!lctrl)
    return;

#define L lctrl->L
  lua_rawgeti(L, LUA_REGISTRYINDEX, lctrl->init_ref);
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    LOG_ERROR("failed to execute lua callback: %s", err);
    lua_pop(L, 1);
  }
#undef L
}

DEFINE_CONTROLLER_DEINIT_FN(Lua) {
  LuaController* lctrl = (LuaController*)data;
  if (!lctrl)
    return;

#define L lctrl->L
  lua_rawgeti(L, LUA_REGISTRYINDEX, lctrl->deinit_ref);
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    LOG_ERROR("failed to execute lua callback: %s", err);
    lua_pop(L, 1);
  }
#undef L
}

static inline void FreeLuaCtrl(void* data) {
  return FreeLuaController((LuaController*)data);
}

Controller* NewLuaController(lua_State* L, const char* kind, const int tbl_index) {
  if (!L)
    return NULL;
  LuaController* lctrl = (LuaController*)malloc(sizeof(LuaController));
  memset(lctrl, 0, sizeof(LuaController));
  lctrl->L = L;

  lua_pushvalue(L, tbl_index);
  if (!lua_istable(L, -1)) {
    luaL_error(L, "expected argument #2 to be a table");
    lua_pop(L, 1);
    return NULL;
  }

  BindCallbackField(L, -1, "init", &lctrl->init_ref);
  BindCallbackField(L, -1, "deinit", &lctrl->deinit_ref);
  BindCallbackField(L, -1, "observe", &lctrl->observe_ref);
  BindCallbackField(L, -1, "plan", &lctrl->plan_ref);
  BindCallbackField(L, -1, "apply", &lctrl->apply_ref);
  BindCallbackField(L, -1, "destroy", &lctrl->destroy_ref);
  BindCallbackField(L, -1, "validate", &lctrl->validate_ref);
  BindCallbackField(L, -1, "diff", &lctrl->diff_ref);
  BindCallbackField(L, -1, "status", &lctrl->status_ref);
  BindCallbackField(L, -1, "rollback", &lctrl->rollback_ref);
  BindCallbackField(L, -1, "normalize", &lctrl->normalize_ref);
  lua_pop(L, 1);

  ResourceKind k = NewResourceKind(kind);
  if (k == kInvalidResourceKind)
    return NULL;
  ControllerConfig config = {
      .init = LuaInit,
      .deinit = LuaDeInit,
  };
  return NewController(k, config, NULL, 0, lctrl, &FreeLuaCtrl);
}

void FreeLuaController(LuaController* ctrl) {
  if (ctrl)
    return;
#define L ctrl->L
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->init_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->deinit_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->observe_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->plan_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->apply_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->destroy_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->validate_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->diff_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->status_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->rollback_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, ctrl->normalize_ref);
#undef L
  free(ctrl);
}
