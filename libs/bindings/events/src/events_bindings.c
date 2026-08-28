#include "hypha/events_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/event.h"
#include "hypha/log.h"

typedef struct {
  lua_State* L;
  int ref;
} LuaCallback;

static inline LuaCallback* NewLuaCallback(lua_State* L, const int ref) {
  LuaCallback* cb = (LuaCallback*)malloc(sizeof(LuaCallback));
  if (cb) {
    cb->L = L;
    cb->ref = ref;
  }

  return cb;
}

#define LUA_TO_TABLE_FN(Name) static inline int Name##ToTable(lua_State* L, const char* name, Name##Event* event)

#define SET_EVENT_FIELDS(Index)                    \
  lua_pushstring(L, name);                         \
  lua_setfield(L, (Index), "name");                \
  lua_pushnumber(L, (lua_Number)event->timestamp); \
  lua_setfield(L, (Index), "timestap");

LUA_TO_TABLE_FN(GraphSubmitted) {
  lua_newtable(L);
  SET_EVENT_FIELDS(-2);
  return 1;
}

// ╭──────────────────╮
// │ Reconcile Events │
// ╰──────────────────╯
LUA_TO_TABLE_FN(ReconcileStarted) {
  lua_newtable(L);
  SET_EVENT_FIELDS(-2);
  return 1;
}

#define DEFINE_RECONCILE_EVENT_TOTABLE(Name)                     \
  LUA_TO_TABLE_FN(Reconcile##Name) {                             \
    lua_newtable(L);                                             \
    SET_EVENT_FIELDS(-2);                                        \
    lua_pushstring(L, ControllerStatusToCString(event->status)); \
    lua_setfield(L, -2, "status");                               \
    return 1;                                                    \
  }

DEFINE_RECONCILE_EVENT_TOTABLE(Complete);
DEFINE_RECONCILE_EVENT_TOTABLE(Failed);
DEFINE_RECONCILE_EVENT_TOTABLE(Finished);
#undef DEFINE_RECONCILE_EVENT_TOTABLE
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────────╮
// │ Orchestrator Events │
// ╰─────────────────────╯
#define DEFINE_ORCHESTRATOR_EVENT_TOTABLE(Name) \
  LUA_TO_TABLE_FN(Name) {                       \
    lua_newtable(L);                            \
    SET_EVENT_FIELDS(-2);                       \
    return 1;                                   \
  }

FOR_EACH_ORCHESTRATOR_EVENT(DEFINE_ORCHESTRATOR_EVENT_TOTABLE);
#undef DEFINE_ORCHESTRATOR_EVENT_TOTABLE
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────╮
// │ Resource Events │
// ╰─────────────────╯
#define DEFINE_RESOURCE_EVENT_TOTABLE(Name) \
  LUA_TO_TABLE_FN(Name) {                   \
    lua_newtable(L);                        \
    SET_EVENT_FIELDS(-2);                   \
    return 1;                               \
  }

FOR_EACH_RESOURCE_EVENT(DEFINE_RESOURCE_EVENT_TOTABLE)
#undef DEFINE_RESOURCE_EVENT_TOTABLE
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────╮
// │ Rollback Events │
// ╰─────────────────╯
#define DEFINE_ROLLBACK_EVENT_TOTABLE(Name) \
  LUA_TO_TABLE_FN(Name) {                   \
    lua_newtable(L);                        \
    SET_EVENT_FIELDS(-2);                   \
    return 1;                               \
  }

FOR_EACH_ROLLBACK_EVENT(DEFINE_ROLLBACK_EVENT_TOTABLE)
#undef DEFINE_ROLLBACK_EVENT_TOTABLE
// ──────────────────────────────────────────────────────────────────────

#undef LUA_TO_TABLE_FN

static inline int ToTable(lua_State* L, const char* name, const void* event) {
  EventKind kind = *((EventKind*)event);

  switch (kind) {
#define DEFINE_TO_TABLE(Name)                             \
  case k##Name##Event:                                    \
    return Name##ToTable(L, name, ((Name##Event*)event)); \
    break;

    FOR_EACH_EVENT(DEFINE_TO_TABLE)
#undef DEFINE_TO_TABLE
    default:
      return 0;
  }
}

#define L cb->L

static inline bool OnEvent(const char* p, const void* event, void* data) {
  LuaCallback* cb = (LuaCallback*)data;
  if (cb) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, cb->ref);
    const int args = ToTable(L, p, event);
    if (lua_pcall(L, args, 0, 0) != LUA_OK) {
      const char* err = lua_tostring(L, -1);
      LOG_ERROR("failed to execute lua callback: %s", err);
      lua_pop(L, 1);
    }
  }
  return true;
}

static inline void FreeLuaCallback(void* data) {
  LuaCallback* cb = (LuaCallback*)data;
  if (!cb)
    return;

  luaL_unref(L, LUA_REGISTRYINDEX, cb->ref);
  free(cb);
}

#undef L

LUA_FN(on) {
  const char* p = luaL_checkstring(L, 1);

  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushvalue(L, 2);
  const int cb = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_getfield(L, LUA_REGISTRYINDEX, LUA_REGISTRY_EVENTS_KEY);
  if (!lua_islightuserdata(L, -1))
    return luaL_typeerror(L, -1, "error: expected to be lightuserdata");

  EventBus* bus = (EventBus*)lua_touserdata(L, -1);
  if (!bus)
    return luaL_error(L, "error: expected '%s' registry key to not be nil", LUA_REGISTRY_EVENTS_KEY);

  EventBusSubscribe(bus, p, &OnEvent, NewLuaCallback(L, cb), &FreeLuaCallback);
  return 0;
}

static const size_t kNumberOfReservedEvents = 19;
static const char* kReservedEvents[] = {
    GRAPH_SUBMITTED_EVENT,    ORCHESTRATOR_INIT_EVENT,   ORCHESTRATOR_DEINIT_EVENT, RECONCILE_STARTED_EVENT,
    RECONCILE_COMPLETE_EVENT, RECONCILE_FAILED_EVENT,    RECONCILE_FINISHED_EVENT,  RESOURCE_OBSERVED_EVENT,
    RESOURCE_PLANNED_EVENT,   RESOURCE_NORMALIZED_EVENT, RESOURCE_VALIDATED_EVENT,  RESOURCE_APPLIED_EVENT,
    RESOURCE_READY_EVENT,     RESOURCE_ROLLBACK_EVENT,   RESOURCE_DESTROYED_EVENT,  ROLLBACK_STARTED_EVENT,
    ROLLBACK_FAILED_EVENT,    ROLLBACK_COMPLETE_EVENT,   ROLLBACK_FINISHED_EVENT,
};

LUA_FN(emit) {
  const char* p = luaL_checkstring(L, 1);

  for (size_t i = 0; i < kNumberOfReservedEvents; i++) {
    if (strcmp(p, kReservedEvents[i]) == 0)
      return luaL_error(L, "cannot emit '%s' event from lua", p);
  }

  lua_getfield(L, LUA_REGISTRYINDEX, LUA_REGISTRY_EVENTS_KEY);
  if (!lua_islightuserdata(L, -1))
    return luaL_typeerror(L, -1, "error: expected to be lightuserdata");

  EventBus* bus = (EventBus*)lua_touserdata(L, -1);
  if (!bus)
    return luaL_error(L, "error: expected '%s' registry key to not be nil", LUA_REGISTRY_EVENTS_KEY);

  EventBusPublish(bus, p, NULL);
  return 0;
}

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(on)
  BIND(emit)
  // TODO(@s0cks): implement BIND(emit)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_events, kFuncs);
