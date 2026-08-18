#ifndef HYPHA_DISCOVERY_H
#define HYPHA_DISCOVERY_H

#include <lua.h>
#include <stddef.h>

#include "hypha.h"

typedef struct {
  DiscoveredManifestKind kind;
  char* value;
} DiscoveredManifest;

void DiscoverManifestPaths(lua_State* L, DiscoveredManifest** discovered, size_t* num_discovered);

#endif  // HYPHA_DISCOVERY_H
