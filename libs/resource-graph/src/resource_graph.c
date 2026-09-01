#include "hypha/resource_graph.h"

#include <stdlib.h>

#include "hypha.h"
#include "hypha/assertions.h"
#include "hypha/bitset.h"
#include "hypha/log.h"
#include "hypha/resource_selector.h"

struct _ResourceGraph {
  Resource* resources;
  uint64_t count;
  uint64_t capacity;

  ResourceGraphIndex* execution_order;
};

void EnsureCapacity(ResourceGraph* graph);

Resource* GetResourceGraphResources(ResourceGraph* rg) {
  return rg ? rg->resources : NULL;
}

uint64_t GetNumberOfResourcesInResourceGraph(ResourceGraph* rg) {
  return rg ? rg->count : 0;
}

Resource* FindResourceMatching(const ResourceGraph* rg, ResourceSelector* selector) {
  ASSERT(rg);
  ASSERT(selector);

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    Resource* res = &rg->resources[i];
    if (ResourceSelectorMatch(selector, res))
      return res;
  }

  return NULL;
}

bool VisitAllResources(const ResourceGraph* rg, ResourceVisitorFn fn, void* data) {
  if (!rg)
    return false;

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    Resource* res = &rg->resources[i];
    if (!fn(i, res, data))
      return false;
  }

  return true;
}

bool VisitAllNonMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn,
                                  void* data) {
  if (!rg || !rs)
    return false;

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    Resource* res = &rg->resources[i];
    if (ResourceSelectorMatch(rs, res))
      continue;

    if (!fn(i, res, data))
      return false;
  }

  return true;
}

bool VisitAllMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn, void* data) {
  if (!rg || !rs)
    return false;

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    Resource* res = &rg->resources[i];
    if (!ResourceSelectorMatch(rs, res))
      continue;

    if (!fn(i, res, data))
      return false;
  }

  return true;
}

ResourceGraph* NewResourceGraph() {
  static const size_t kInitCapacity = 8;
  ResourceGraph* result = NULL;  // NOLINT(modernize-use-nullptr)

  ResourceGraph* graph = (ResourceGraph*)malloc(sizeof(ResourceGraph));
  if (!graph)
    goto finished;

  Resource* resources = (Resource*)malloc(sizeof(Resource) * kInitCapacity);
  if (!resources)
    goto failed0;

  graph->count = 0;
  graph->capacity = kInitCapacity;
  graph->resources = resources;
  graph->execution_order = NULL;
  result = graph;
  goto finished;
failed0:
  free(graph);
finished:
  return result;
}

void EnsureCapacity(ResourceGraph* graph) {
  if (!graph || graph->count < graph->capacity)
    goto finished;

  const size_t new_cap = graph->capacity * 2;
  Resource* new_resources = realloc(graph->resources, sizeof(Resource) * new_cap);
  if (!new_resources) {
    LOG_ERROR("failed to allocate new resources for ResourceGraph with capacity %d", new_cap);
    goto finished;
  }

  graph->capacity = new_cap;
  graph->resources = new_resources;
finished:
  return;
}

static inline ResourceGraphIndex FindResourceIndex(ResourceGraph* graph, const char* ref) {
  ASSERT(graph);
  ASSERT(ref);

  // depends_on entries may reference either a resource's name (the expected case,
  // since that's what a manifest author writes) or its id (in case something
  // machine-generated the reference). Id is authoritative, so try it first when
  // it parses as one; fall back to a name scan either way.
  ResourceId needle;
  if (uuid_parse(ref, needle) == 0) {
    for (ResourceGraphIndex i = 0; i < graph->count; i++) {
      if (uuid_compare(graph->resources[i].id, needle) == 0)
        return (ResourceGraphIndex)i;
    }
  }

  for (ResourceGraphIndex i = 0; i < graph->count; i++) {
    const char* name = graph->resources[i].info.name;
    if (name && strcmp(name, ref) == 0)
      return (ResourceGraphIndex)i;
  }

  return kInvalidResourceIndex;
}

bool ComputeExecutionSchedule(ResourceGraph* graph, const SchedulingStrategy strategy) {
  if (IsResourceGraphEmpty(graph))
    return true;

  switch (strategy) {
    case kPriorityWeightedKahnScheduling:
      return ComputeSchedulePriorityWeightedKahn(graph->resources, graph->count, &graph->execution_order);
    case kDepthFirstScheduling:
    default:
      return ComputeScheduleDepthFirst(graph->resources, graph->count, &graph->execution_order);
  }
}

bool DependenciesAreSatisfied(ResourceGraph* graph, Resource* res) {
  for (ResourceGraphIndex i = 0; i < res->num_depends_on; i++) {
    ResourceGraphIndex dep_idx = FindResourceIndex(graph, res->depends_on[i]);
    if (dep_idx == kInvalidResourceIndex || !IsResourceReady(&graph->resources[dep_idx]))
      return false;
  }

  return true;
}

void FreeResourceGraph(ResourceGraph* graph) {
  if (!graph)
    return;

  for (ResourceGraphIndex i = 0; i < graph->count; i++) {
    Resource* res = &graph->resources[i];

    for (ResourceGraphIndex j = 0; j < res->num_depends_on; j++) {
      if (res->depends_on[j])
        free(res->depends_on[j]);
    }

    if (res->depends_on)
      free(res->depends_on);
  }

  if (graph->resources)
    free(graph->resources);

  if (graph->execution_order)
    free(graph->execution_order);

  free(graph);
}

Resource* AllocNewResouceInGraph(ResourceGraph* rg) {
  if (!rg)
    return NULL;

  EnsureCapacity(rg);
  Resource* new_res = &rg->resources[rg->count];
  ASSERT(new_res);
  rg->count++;
  memset(new_res, 0, sizeof(Resource));
  return new_res;
}

ResourceGraphIndex ResourceGraphGetAtOrderIndex(ResourceGraph* graph, const ResourceGraphIndex idx) {
  ASSERT(graph);
  return graph->execution_order[idx];
}

typedef struct {
  uuid_t target;
  Resource* value;
} MatchResult;

static inline bool MatchResourceToId(const ResourceGraphIndex idx, Resource* res, void* data) {
  ASSERT(res);
  MatchResult* result = (MatchResult*)data;
  if (uuid_compare(res->id, result->target) != 0)
    return true;  // skip

  result->value = res;
  return false;  // finish iterating
}

const char* FindNameForResourceId(const ResourceGraph* gr, const uuid_t id) {
  ASSERT(gr);
  MatchResult result;
  uuid_copy(result.target, id);
  VisitAllResources(gr, &MatchResourceToId, (void*)&result);

  Resource* value = result.value;
  if (!value)
    return NULL;

  return value->info.name;
}

Resource* GetResourceInGraph(ResourceGraph* rg, const uint64_t idx) {
  if (!rg || idx >= rg->count)
    return NULL;
  return &rg->resources[idx];
}
