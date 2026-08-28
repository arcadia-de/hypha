#include "hypha/query_bindings.h"

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

LUA_FN(query) {
  const char* query_text = luaL_checkstring(L, 1);

  lua_getfield(L, LUA_REGISTRYINDEX, LUA_REGISTRY_ORC_KEY);
  Orchestrator* orc = (Orchestrator*)lua_touserdata(L, -1);
  ResourceGraph* graph = GetOrcResourceGraph(orc);

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

// clang-format off
static const struct luaL_Reg kFuncs[] = {
#define BIND(Name) {#Name, lua_##Name},
  BIND(query)
#undef BIND
  {NULL, NULL},  // NOLINT(modernize-use-nullptr)
};
// clang-format on

DEFINE_LUA_BINDINGS(hypha_resource, kFuncs);
