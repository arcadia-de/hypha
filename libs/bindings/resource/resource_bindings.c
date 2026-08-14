#include "resource_bindings.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>

#include "hypha.h"
#include "hypha/orchestrator.h"
#include "hypha/query.h"
#include "hypha/resource_query_schema.h"

static inline void PushResultNode(lua_State* L, const QueryResult* node) {
  switch (node->kind) {
    case kQueryResultNull:
      lua_pushnil(L);
      break;

    case kQueryResultString:
      lua_pushstring(L, node->string_value);
      break;

    case kQueryResultArray:
      lua_createtable(L, (int)node->num_array_items, 0);
      for (uint32_t i = 0; i < node->num_array_items; i++) {
        PushResultNode(L, node->array_items[i]);
        lua_rawseti(L, -2, (int)i + 1);  // Lua arrays are 1-indexed
      }
      break;

    case kQueryResultObject:
      lua_createtable(L, 0, (int)node->num_object_fields);
      for (uint32_t i = 0; i < node->num_object_fields; i++) {
        PushResultNode(L, node->object_fields[i].value);
        lua_setfield(L, -2, node->object_fields[i].key);
      }
      break;
    default:
      break;
  }
}

static inline int LuaQuery(lua_State* L) {
  const char* query_text = luaL_checkstring(L, 1);

  lua_getfield(L, LUA_REGISTRYINDEX, CONTEXT_REGISTRY_KEY_ORCHESTRATOR);
  OrchestratorHandle orc = (OrchestratorHandle*)lua_touserdata(L, -1);
  ResourceGraph* graph = OrchestratorGetResourceGraph(orc);

  if (!graph) {
    lua_pushnil(L);
    lua_pushstring(L, "cannot get ResourceGraph from orchestrator");
    return 2;
  }

  ResourcesQueryContext ctx = {
      .resources = GetResourceGraphResources(graph),
      .count = GetNumberOfResourcesInResourceGraph(graph),
  };
  QuerySchema schema = HyphaResourcesQuerySchema(&ctx);

  char* err = NULL;
  QueryResult* result = QueryExecute(&schema, query_text, &err);
  if (!result) {
    lua_pushnil(L);
    lua_pushstring(L, err);
    free(err);
    return 2;
  }

  PushResultNode(L, result);
  ResultNodeFree(result);
  return 1;
}

static const struct luaL_Reg kFuncs[] = {
    {"query", LuaQuery},
    {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};

DEFINE_LUA_BINDINGS(hypha_resource, kFuncs);
