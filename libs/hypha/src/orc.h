#ifndef HYPHA_ORC_H
#define HYPHA_ORC_H

#include "hypha.h"
#include "hypha/controller.h"
#include "hypha/discovery.h"
#include "hypha/event.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/planner.h"
#include "hypha/resource_decorator.h"
#include "hypha/resource_graph.h"
#include "hypha/run_info.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"
#include "reconcile.h"

#ifdef HYPHA_ENABLE_PROFILING
#include <tracy/tracy/TracyC.h>
#endif  // HYPHA_ENABLE_PROFILING

#ifndef UV_OK
#define UV_OK 0
#endif  // UV_OK

struct _Orchestrator {
  OrchestratorConfig config;
  RunInfo run;

  uv_loop_t* loop;
#ifdef HYPHA_ENABLE_PROFILING
  uv_check_t profiling_check;
#endif  // HYPHA_ENABLE_PROFILING

  ResourceGraph* graph;
  EventBus* bus;
  lua_State* L;
  uint32_t pending;
  StateStore* state;
  HistoryLog* history;
  Plan plan;
  DeltaLog dlog;
  ValidationLog vlog;
  OrchestratorMetrics metrics;
  ResourceDecoratorPipeline decorator;
  ActionLog actions;

  size_t num_discovered_manifests;
  DiscoveredManifest* discovered_manifests;
};

lua_State* NewOrchestratorLuaState(Orchestrator*);
void QueueReconcileTask(Orchestrator* orc, Controller* ctrl, const ResourceGraphIndex index, Resource* res);
void DispatchReadyResources(Orchestrator* orc);

#endif  // HYPHA_ORC_H
