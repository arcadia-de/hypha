#include "hypha/discovery.h"

#include <lua.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/dedupe.h"
#include "hypha/log.h"
#include "hypha/package_manager.h"

static inline int CompareDiscoveredManifestKind(const void* lhs, const void* rhs) {
  const DiscoveredManifest* a = lhs;
  const DiscoveredManifest* b = rhs;

  if (a->kind < b->kind)
    return -1;
  else if (a->kind > b->kind)
    return +1;
  return 0;
}

static inline int CompareDiscoveredManifestPath(DiscoveredManifest* lhs, DiscoveredManifest* rhs) {
  return strcmp(lhs->value, rhs->value);
}

static inline void FreeDiscoveredManifest(void* data) {
  if (!data)
    return;

  DiscoveredManifest* rhs = (DiscoveredManifest*)data;

  if (rhs->value)
    free(rhs->value);
}

static inline void DedupeDiscoveredManifests(DiscoveredManifest* arr, size_t size, size_t* new_size,
                                             void free_data(void*)) {
  if (arr == NULL || size == 0)
    return;

  for (size_t i = 0; i < size; i++) {
    for (size_t j = i + 1; j < size; j++) {
      if (arr[i].kind == kDiscoveredRaw)
        continue;

      if (CompareDiscoveredManifestPath(&arr[i], &arr[j]) == 0) {
        free_data(&arr[j]);
        arr[j] = arr[size - 1];
        size--;
        j--;
      }
    }
  }
  *new_size = size;
}

void DiscoverManifestPaths(lua_State* L, DiscoveredManifest** discovered, size_t* num_discovered) {
  size_t capacity = 0;
  DiscoveredManifest* values = NULL;
  size_t num_values = 0;

  const int stack_size = lua_gettop(L);
  if (stack_size > 0) {
    if (!lua_isnil(L, -1) && !lua_istable(L, -1)) {
      LOG_ERROR("expected the config to return nil or a table, received: %s", lua_typename(L, lua_type(L, -1)));
    } else if (lua_istable(L, -1)) {
      const size_t len = lua_rawlen(L, -1);
      if (len > 0) {
        capacity = len;
        values = (DiscoveredManifest*)malloc(sizeof(DiscoveredManifest) * capacity);
        memset(values, 0, sizeof(DiscoveredManifest) * capacity);
      }

      for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, (lua_Integer)i);

        if (lua_istable(L, -1)) {
          const size_t sub_len = lua_rawlen(L, -1);
          if (num_values + sub_len > capacity) {
            capacity = num_values + sub_len;
            values = (DiscoveredManifest*)realloc(values, sizeof(DiscoveredManifest) * capacity);
          }

          if (sub_len == 0 && lua_istable(L, -1)) {
            if (num_values + 1 > capacity) {
              capacity = num_values + 1;
              values = (DiscoveredManifest*)realloc(values, sizeof(DiscoveredManifest) * capacity);
            }

            DiscoveredManifest* value = &values[num_values];

            lua_getfield(L, -1, "kind");
            value->kind = (DiscoveredManifestKind)lua_tonumber(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "value");
            const char* str_val = lua_tostring(L, -1);
            value->value = str_val ? strdup(str_val) : NULL;
            lua_pop(L, 1);

            num_values++;
          }

          for (size_t j = 1; j <= sub_len; j++) {
            lua_rawgeti(L, -1, (lua_Integer)j);

            if (lua_istable(L, -1)) {
              DiscoveredManifest* value = &values[num_values];

              lua_getfield(L, -1, "kind");
              value->kind = (DiscoveredManifestKind)lua_tonumber(L, -1);
              lua_pop(L, 1);

              lua_getfield(L, -1, "value");
              const char* str_val = lua_tostring(L, -1);
              value->value = str_val ? strdup(str_val) : NULL;
              lua_pop(L, 1);

              num_values++;
            } else if (lua_isstring(L, -1)) {
              DiscoveredManifest* value = &values[num_values];
              value->kind = kDiscoveredPath;
              value->value = strdup(lua_tostring(L, -1));
            } else {
              DLOG_WARN("expected subtable value at array index %zu to be a table record", j);
            }

            lua_pop(L, 1);
          }

          lua_pop(L, 1);

        } else if (lua_isstring(L, -1)) {
          if (num_values >= capacity) {
            capacity = num_values + 1;
            values = (DiscoveredManifest*)realloc(values, sizeof(DiscoveredManifest) * capacity);
          }

          DiscoveredManifest* value = &values[num_values];
          value->kind = kDiscoveredPath;
          value->value = strdup(lua_tostring(L, -1));
          num_values++;
          lua_pop(L, 1);
        } else {
          DLOG_WARN("expected table value at %zu to be a string or table", i);
          lua_pop(L, 1);
        }
      }
    }
  }

  qsort(values, num_values, sizeof(DiscoveredManifest), &CompareDiscoveredManifestKind);
  DedupeDiscoveredManifests(values, num_values, &num_values, FreeDiscoveredManifest);

  (*discovered) = values;
  (*num_discovered) = num_values;
}
