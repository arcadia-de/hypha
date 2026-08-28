#include "hypha/assertions.h"
#include "hypha/resource_graph.h"

typedef struct {
  ResourceGraphIndex* edges;
  size_t count;
  size_t capacity;
} AdjacencyList;

static inline void FreeAdjacencyList(AdjacencyList* adj, const size_t num_resources) {
  if (!adj)
    return;

  for (size_t i = 0; i < num_resources; i++)
    free(adj[i].edges);

  free(adj);
}

static inline ResourceGraphIndex FindResourceIndexByRef(const Resource* resources, size_t num_resources,
                                                        const char* ref) {
  if (!ref)
    return kInvalidResourceIndex;

  for (size_t i = 0; i < num_resources; i++) {
    const Resource* res = &resources[i];
    if (ResourceMatchesRef(res, ref))
      return (ResourceGraphIndex)i;
  }

  return kInvalidResourceIndex;
}

static inline Priority GetResourcePriorityWeight(const Resource* res) {
  ASSERT(res);
  return res->priority;
}

bool ComputeSchedulePriorityWeightedKahn(const Resource* resources, const size_t num_resources,
                                         ResourceGraphIndex** results) {
  if (!resources || num_resources == 0 || !results)
    return false;

  ResourceGraphIndex* out_schedule = (ResourceGraphIndex*)malloc(num_resources * sizeof(ResourceGraphIndex));
  if (!out_schedule)
    return false;

  size_t* in_degree = (size_t*)calloc(num_resources, sizeof(size_t));
  AdjacencyList* adj = (AdjacencyList*)calloc(num_resources, sizeof(AdjacencyList));

  if (!in_degree || !adj) {
    free(out_schedule);
    free(in_degree);
    free(adj);
    return false;
  }

  for (size_t i = 0; i < num_resources; ++i) {
    for (size_t d = 0; d < resources[i].num_depends_on; ++d) {
      const char* dependency_ref = resources[i].depends_on[d];
      ResourceGraphIndex dep_idx = FindResourceIndexByRef(resources, num_resources, dependency_ref);

      if (dep_idx != kInvalidResourceIndex) {
        AdjacencyList* list = &adj[dep_idx];
        if (list->count >= list->capacity) {
          list->capacity = list->capacity == 0 ? 4 : list->capacity * 2;
          ResourceGraphIndex* new_edges =
              (ResourceGraphIndex*)realloc(list->edges, list->capacity * sizeof(ResourceGraphIndex));

          if (!new_edges)
            goto cleanup_failure;

          list->edges = new_edges;
        }

        list->edges[list->count++] = i;
        in_degree[i]++;
      }
    }
  }

  size_t ready_count = 0;
  ResourceGraphIndex* ready_pool = (ResourceGraphIndex*)malloc(num_resources * sizeof(ResourceGraphIndex));
  if (!ready_pool)
    goto cleanup_failure;

  for (size_t i = 0; i < num_resources; ++i) {
    if (in_degree[i] == 0)
      ready_pool[ready_count++] = i;
  }

  size_t scheduled_count = 0;
  while (ready_count > 0) {
    size_t target_pool_idx = 0;
    Priority max_priority = GetResourcePriorityWeight(&resources[ready_pool[0]]);

    for (size_t i = 1; i < ready_count; ++i) {
      Priority current_priority = GetResourcePriorityWeight(&resources[ready_pool[i]]);
      if (current_priority > max_priority) {
        max_priority = current_priority;
        target_pool_idx = i;
      }
    }

    ResourceGraphIndex current_res_idx = ready_pool[target_pool_idx];
    ready_pool[target_pool_idx] = ready_pool[ready_count - 1];
    ready_count--;

    out_schedule[scheduled_count++] = current_res_idx;

    AdjacencyList* list = &adj[current_res_idx];
    for (size_t e = 0; e < list->count; ++e) {
      ResourceGraphIndex neighbor_idx = list->edges[e];
      in_degree[neighbor_idx]--;

      if (in_degree[neighbor_idx] == 0) {
        ready_pool[ready_count++] = neighbor_idx;
      }
    }
  }

  free(ready_pool);
  FreeAdjacencyList(adj, num_resources);
  free(in_degree);

  if (scheduled_count != num_resources) {
    free(out_schedule);
    *results = NULL;
    return false;
  }

  *results = out_schedule;
  return true;

cleanup_failure:
  free(out_schedule);
  free(in_degree);
  FreeAdjacencyList(adj, num_resources);
  if (ready_pool)
    free(ready_pool);
  *results = NULL;
  return false;
}
