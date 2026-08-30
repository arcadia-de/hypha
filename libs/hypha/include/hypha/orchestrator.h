#ifndef HYPHA_ORCHESTRATOR_H
#define HYPHA_ORCHESTRATOR_H

#include <lua.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller.h"
#include "hypha/delta_log.h"
#include "hypha/discovery.h"
#include "hypha/event.h"
#include "hypha/planner.h"
#include "hypha/resource_decorator.h"
#include "hypha/resource_graph.h"
#include "hypha/run_info.h"
#include "hypha/run_mode.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

#ifdef HYPHA_ENABLE_PROFILING
#include <tracy/tracy/TracyC.h>
#endif  // HYPHA_ENABLE_PROFILING

#ifndef UV_OK
#define UV_OK 0
#endif  // UV_OK

typedef struct {
  char* root;
  char* state_dir;
  char* cache_dir;
} OrchestratorConfig;

typedef struct {
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
  AppliedActionLog actions;

  size_t num_discovered_manifests;
  DiscoveredManifest* discovered_manifests;
} Orchestrator;

void GetOrchestratorMetrics(OrchestratorHandle orc, OrchestratorMetrics* out);

OrchestratorHandle NewOrchestrator(OrchestratorConfig config);
ResourceGraph* GetOrcResourceGraph(OrchestratorHandle);
HistoryLog* GetOrcHistoryLog(OrchestratorHandle);
lua_State* GetOrcLuaState(OrchestratorHandle);
EventRoute* GetOrcRootEventRoute(OrchestratorHandle);
EventBus* GetOrcEventBus(OrchestratorHandle);
ValidationLog* GetOrcValidationLog(OrchestratorHandle);
AppliedActionLog* GetOrcAppliedActionLog(OrchestratorHandle);
Plan* GetOrcPlan(OrchestratorHandle);
const char* GetOrcConfigDir(OrchestratorHandle);
const char* GetOrcStateDir(OrchestratorHandle);
const char* GetOrcCacheDir(OrchestratorHandle);
StateStore* GetOrcStateStore(OrchestratorHandle);

lua_State* NewOrchestratorLuaState(Orchestrator*);
void QueueReconcileTask(Orchestrator* orc, Controller* ctrl, const ResourceGraphIndex index, Resource* res);
void DispatchReadyResources(Orchestrator* orc);

void OrchestratorSubscribe(OrchestratorHandle, const char* p, EventCallbackFn cb, void* data, void (*free_data)(void*));
void OrchestratorPublish(OrchestratorHandle, const char* p, void* event);
bool OrchestratorRun(OrchestratorHandle, RunInfo* info);
bool OrchestratorPruneOrphans(OrchestratorHandle);
bool OrchestratorCompact(OrchestratorHandle);
bool OrchestratorEvalExpr(OrchestratorHandle, const char* expr, char** err);
bool OrchestratorEvalFile(OrchestratorHandle, const char* filename, char** err);
void FreeOrchestrator(OrchestratorHandle);
void OrchestratorPrintRuntimeInfo(OrchestratorHandle);

typedef bool (*VisitDiscoveredManifestFn)(const uint64_t, DiscoveredManifest*, void*);
void VisitDiscoveredManifests(OrchestratorHandle, VisitDiscoveredManifestFn, void* data);

#ifdef HYPHA_GRAPHVIZ_ENABLED
void OrchestratorRenderResourceGraphTo(OrchestratorHandle, const char* name, const char* layout, const char* render,
                                       FILE* stream);

static inline void OrchestratorRenderResourceGraphToStdout(OrchestratorHandle handle, const char* name,
                                                           const char* layout, const char* render) {
  return OrchestratorRenderResourceGraphTo(handle, name, layout, render, stdout);
}
#endif  // HYPHA_GRAPHVIZ_ENABLED

#endif  // HYPHA_ORCHESTRATOR_H
