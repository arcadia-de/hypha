#include "hypha/assertions.h"
#include "hypha/resource_graph.h"

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

static inline void PropagatePriorityToDependencies(Resource* resources, const size_t num_resources) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (uint32_t i = 0; i < num_resources; i++) {
      const Resource* res = &resources[i];
      for (uint32_t j = 0; j < res->num_depends_on; j++) {
        const ResourceGraphIndex dep_idx = FindResourceIndexByRef(resources, num_resources, res->depends_on[j]);
        if (dep_idx == kInvalidResourceIndex)
          continue;

        Resource* dep = &resources[dep_idx];
        if (res->priority > dep->priority) {
          dep->priority = res->priority;
          changed = true;
        }
      }
    }
  }
}

bool ComputeSchedulePriorityWeightedKahn(Resource* resources, const size_t num_resources,
                                         ResourceGraphIndex** results) {
  if (!resources || num_resources == 0)
    return true;

  PropagatePriorityToDependencies(resources, num_resources);

  uint32_t* in_degree = (uint32_t*)malloc(sizeof(uint32_t) * num_resources);
  bool* scheduled = (bool*)calloc(num_resources, sizeof(bool));
  if (!in_degree || !scheduled) {
    free(in_degree);
    free(scheduled);
    return false;
  }

  for (uint32_t i = 0; i < num_resources; i++)
    in_degree[i] = resources[i].num_depends_on;

  uint32_t output_len = 0;
  bool ok = true;

  ResourceGraphIndex* order = (ResourceGraphIndex*)malloc(sizeof(ResourceGraphIndex) * num_resources);

  while (output_len < num_resources) {
    int32_t best = -1;
    for (uint32_t i = 0; i < num_resources; i++) {
      if (scheduled[i] || in_degree[i] != 0)
        continue;
      if (best == -1 || resources[i].priority > resources[(uint32_t)best].priority)
        best = (int32_t)i;
    }

    if (best == -1) {
      ok = false;
      break;
    }

    order[output_len++] = best;
    scheduled[(uint32_t)best] = true;

    const char* best_id = resources[(uint32_t)best].id;
    for (uint32_t i = 0; i < num_resources; i++) {
      if (scheduled[i])
        continue;

      const Resource* res = &resources[i];
      for (uint32_t j = 0; j < res->num_depends_on; j++) {
        if (strcmp(res->depends_on[j], best_id) == 0) {
          in_degree[i]--;
          break;
        }
      }
    }
  }

  free(in_degree);
  free(scheduled);
  return ok;
}
