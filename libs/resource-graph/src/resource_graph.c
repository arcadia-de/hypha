#include "hypha/resource_graph.h"

#include <stdlib.h>

#include "hypha.h"
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

bool VisitAllResources(const ResourceGraph* rg, ResourceVisitorFn fn, void* data) {
  if (!rg)
    return false;

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    const Resource* res = &rg->resources[i];
    if (!fn(res, data))
      return false;
  }

  return true;
}

bool VisitAllNonMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn,
                                  void* data) {
  if (!rg || !rs)
    return false;

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    const Resource* res = &rg->resources[i];
    if (ResourceSelectorMatch(rs, res))
      continue;

    if (!fn(res, data))
      return false;
  }

  return true;
}

bool VisitAllMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn, void* data) {
  if (!rg || !rs)
    return false;

  for (ResourceGraphIndex i = 0; i < rg->count; i++) {
    const Resource* res = &rg->resources[i];
    if (!ResourceSelectorMatch(rs, res))
      continue;

    if (!fn(res, data))
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

  ResourceGraphIndex* exec_order = (ResourceGraphIndex*)malloc(sizeof(ResourceGraphIndex) * kInitCapacity);
  if (!exec_order)
    goto failed1;

  graph->count = 0;
  graph->capacity = kInitCapacity;
  graph->resources = resources;
  graph->execution_order = exec_order;
  result = graph;
  goto finished;
failed1:
  free(resources);
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

  ResourceGraphIndex* new_exec_order = realloc(graph->execution_order, sizeof(ResourceGraphIndex) * new_cap);
  if (!new_exec_order) {
    LOG_ERROR("failed to allocate new execution_order for ResourceGraph with capacity %d", new_cap);
    goto failed0;
  }

  graph->capacity = new_cap;
  graph->resources = new_resources;
  graph->execution_order = new_exec_order;
  goto finished;
failed0:
  free(new_resources);
finished:
  return;
}

static inline ResourceGraphIndex FindResourceIndex(ResourceGraph* graph, const char* id) {
  for (ResourceGraphIndex i = 0; i < graph->count; i++) {
    if (strcmp(graph->resources[i].id, id) == 0)
      return (ResourceGraphIndex)i;
  }

  return kInvalidResourceIndex;
}

bool topological_sort_dfs(ResourceGraph* graph, ResourceGraphIndex node_idx, BitSet* visited, BitSet* stack,
                          ResourceGraphIndex* output_idx) {
  if (BitSetTest(stack, node_idx))
    return false;

  if (BitSetTest(visited, node_idx))
    return true;

  BitSetSet(stack, node_idx);
  BitSetSet(visited, node_idx);

  Resource* res = &graph->resources[node_idx];
  for (ResourceGraphIndex i = 0; i < res->num_depends_on; i++) {
    ResourceGraphIndex dep_idx = FindResourceIndex(graph, res->depends_on[i]);
    if (dep_idx == kInvalidResourceIndex) {
      LOG_ERROR("out-of-bounds dependency: '%s' relies on missing '%s'", res->id, res->depends_on[i]);
      return false;
    }

    if (!topological_sort_dfs(graph, dep_idx, visited, stack, output_idx))
      return false;
  }

  BitSetReset(stack, node_idx);
  graph->execution_order[*output_idx] = node_idx;
  (*output_idx)++;
  return true;
}

bool ComputeExecutionSchedule(ResourceGraph* graph) {
  if (IsResourceGraphEmpty(graph))
    return true;

  BitSet stack;
  InitBitSet(&stack, graph->count);
  BitSet visited;
  ResourceGraphIndex output_idx = 0;
  bool success = true;

  for (ResourceGraphIndex i = 0; i < graph->count; i++) {
    if (!BitSetTest(&visited, i)) {
      if (!topological_sort_dfs(graph, i, &visited, &stack, &output_idx)) {
        success = false;
        break;
      }
    }
  }

  FreeBitSet(&stack);
  FreeBitSet(&visited);
  return success;
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
    free(res->id);
    free(res->kind);

    for (ResourceGraphIndex j = 0; j < res->num_depends_on; j++)
      free(res->depends_on[j]);

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

Resource* GetResourceInGraph(ResourceGraph* rg, const uint64_t idx) {
  if (!rg || idx >= rg->count)
    return NULL;
  return &rg->resources[idx];
}
