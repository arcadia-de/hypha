#ifndef HYPHA_RESOURCE_GRAPH_H
#define HYPHA_RESOURCE_GRAPH_H

#include <stdint.h>

#include "hypha.h"
#include "hypha/resource.h"
#include "hypha/resource_selector.h"
#include "hypha/scheduling_strategy.h"

typedef size_t ResourceGraphIndex;
static const ResourceGraphIndex kInvalidResourceIndex = -1;

typedef struct _ResourceGraph ResourceGraph;

ResourceGraph* NewResourceGraph();
Resource* GetResourceGraphResources(ResourceGraph*);
Resource* AllocNewResouceInGraph(ResourceGraph*);
Resource* GetResourceInGraph(ResourceGraph*, const uint64_t idx);
uint64_t GetNumberOfResourcesInResourceGraph(ResourceGraph*);
ResourceGraphIndex ResourceGraphGetAtOrderIndex(ResourceGraph* graph, const ResourceGraphIndex);
bool DependenciesAreSatisfied(ResourceGraph* graph, Resource* res);
bool ComputeExecutionSchedule(ResourceGraph* graph, const SchedulingStrategy strategy);
const char* FindNameForResourceId(const ResourceGraph* gr, const uuid_t id);

#define DECLARE_SCHEDULING_STRATEGY(Name) \
  bool ComputeSchedule##Name(Resource* resources, const size_t num_resources, ResourceGraphIndex** order);
FOR_EACH_SCHEDULING_STRATEGY(DECLARE_SCHEDULING_STRATEGY)
#undef DECLARE_SCHEDULING_STRATEGY

typedef bool (*ResourceVisitorFn)(const ResourceGraphIndex, Resource*, void*);

bool VisitAllResources(const ResourceGraph* rg, ResourceVisitorFn fn, void* data);
bool VisitAllMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn, void* data);
bool VisitAllNonMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn,
                                  void* data);

void FreeResourceGraph(ResourceGraph*);

static inline bool IsResourceGraphEmpty(ResourceGraph* rg) {
  return rg && GetNumberOfResourcesInResourceGraph(rg) == 0;
}

#ifdef HYPHA_GRAPHVIZ_ENABLED

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

bool ResourceGraphToGraphviz(const ResourceGraph* rg, const char* name, Agraph_t** out);
void RenderResourceGraphToGraphvizWithLayout(const ResourceGraph* rg, const char* name, const char* layout,
                                             const char* render, FILE* out);

#endif  // HYPHA_GRAPHVIZ_ENABLED

#endif  // HYPHA_RESOURCE_GRAPH_H
