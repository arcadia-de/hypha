#ifndef HYPHA_RESOURCE_QUERY_SCHEMA_H
#define HYPHA_RESOURCE_QUERY_SCHEMA_H

#include "hypha.h"
#include "hypha/query.h"
#include "hypha/resource.h"

typedef struct {
  Resource* resources;
  uint64_t count;
} ResourcesQueryContext;

QuerySchema* NewHyphaResourcesQuerySchema();

#endif  // HYPHA_RESOURCE_QUERY_SCHEMA_H
