#include "hypha/bitset.h"
#include "hypha/log.h"
#include "hypha/resource_graph.h"

typedef struct {
  BitSet stack;
  BitSet visited;
  ResourceGraphIndex* order;

  const Resource* resources;
  size_t num_resources;
} DfsSchedulerContext;

static inline bool InitDfsSchedulerContext(DfsSchedulerContext* ctx, const Resource* resources,
                                           const size_t num_resources) {
  ctx->resources = resources;
  ctx->num_resources = num_resources;

  {
    const size_t total_size = sizeof(ResourceGraphIndex) * num_resources;
    ResourceGraphIndex* order = (ResourceGraphIndex*)malloc(total_size);
    if (!order)
      return false;
    ctx->order = order;
  }

  InitBitSet(&ctx->stack, num_resources);
  InitBitSet(&ctx->visited, num_resources);
  return true;
}

static inline void FreeDfsSchedulerContext(DfsSchedulerContext* ctx) {
  ASSERT(ctx);
  FreeBitSet(&ctx->stack);
  FreeBitSet(&ctx->visited);
}

static inline ResourceGraphIndex FindResourceIndex(DfsSchedulerContext* ctx, const char* id) {
  ASSERT(ctx);
  ASSERT(id);
  for (ResourceGraphIndex i = 0; i < ctx->num_resources; i++) {
    if (strcmp(ctx->resources[i].id, id) == 0)
      return (ResourceGraphIndex)i;
  }

  return kInvalidResourceIndex;
}

bool topological_sort_dfs(DfsSchedulerContext* ctx, ResourceGraphIndex node_idx, ResourceGraphIndex* output_idx) {
  if (BitSetTest(&ctx->stack, node_idx))
    return false;

  if (BitSetTest(&ctx->visited, node_idx))
    return true;

  BitSetSet(&ctx->stack, node_idx);
  BitSetSet(&ctx->visited, node_idx);

  const Resource* res = &ctx->resources[node_idx];
  for (ResourceGraphIndex i = 0; i < res->num_depends_on; i++) {
    ResourceGraphIndex dep_idx = FindResourceIndex(ctx, res->depends_on[i]);
    if (dep_idx == kInvalidResourceIndex) {
      LOG_ERROR("out-of-bounds dependency: '%s' relies on missing '%s'", res->id, res->depends_on[i]);
      return false;
    }

    if (!topological_sort_dfs(ctx, dep_idx, output_idx))
      return false;
  }

  BitSetReset(&ctx->stack, node_idx);
  ctx->order[*output_idx] = node_idx;
  (*output_idx)++;
  return true;
}

bool ComputeScheduleDepthFirst(const Resource* resources, const size_t num_resources, ResourceGraphIndex** results) {
  ASSERT(resources);
  ASSERT(num_resources > 0);

  DfsSchedulerContext ctx;
  if (!InitDfsSchedulerContext(&ctx, resources, num_resources))
    return false;
  ASSERT(ctx.order);

  ResourceGraphIndex output_idx = 0;
  for (ResourceGraphIndex i = 0; i < num_resources; i++) {
    if (!BitSetTest(&ctx.visited, i)) {
      if (!topological_sort_dfs(&ctx, i, &output_idx))
        return false;
    }
  }

  (*results) = ctx.order;
  FreeDfsSchedulerContext(&ctx);
  return true;
}
