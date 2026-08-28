#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "bootstrap.h"
#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/orchestrator.h"
#include "orc.h"

#ifdef HYPHA_ENABLE_PROFILING

static inline void OnProfilingCheck(uv_check_t* handle) {
  TracyCFrameMark;
}

#endif  // HYPHA_ENABLE_PROFILING

static inline void InitOrcState(Orchestrator* orc) {
  ASSERT(orc->config.state_dir);
  char p[PATH_MAX];
  snprintf(p, PATH_MAX, "%s/journal.log", orc->config.state_dir);
  orc->state = StateStoreOpen(p);
  LOG_FATAL_IF(!orc->state, "failed to open orchestrator state: %s", p);
}

static inline void InitOrcHistory(Orchestrator* orc) {
  ASSERT(orc->config.state_dir);
  char p[PATH_MAX];
  snprintf(p, PATH_MAX, "%s/history.log", orc->config.state_dir);
  orc->history = HistoryLogOpen(p, 5 * 1024 * 1024, 2);
  LOG_FATAL_IF(!orc->history, "failed to open history log: %s", p);
}

lua_State* NewOrchestratorLuaState(Orchestrator* orc);
static inline void InitOrcLuaState(Orchestrator* orc) {
  orc->L = NewOrchestratorLuaState(orc);
  LOG_FATAL_IF(!orc->L, "failed to create orchestrator lua state");
}

static inline bool StopLoopOnReconcileDone(const char* p, const void* event, void* data) {
  ASSERT(event);
  Orchestrator* orc = (Orchestrator*)data;
  uv_stop(orc->loop);
  return true;
}

static inline bool OnReconcileComplete(const char* p, const void* event, void* data) {
  return true;
}

static inline void FailedToExecuteInit(FILE* out, Orchestrator* orc, const char* path) {
  ASSERT(out);
  ASSERT(orc);
  ASSERT(path);
#define L orc->L
  const char* err = lua_tostring(L, -1);
  LOG_FATAL("failed to execute init file %s: %s", path, err);
#undef L
}

static inline void ExecInit(Orchestrator* orc) {
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/init.lua", orc->config.root);
  const int result = luaL_dofile(orc->L, path);  // TODO(@s0cks): check result
  if (result != LUA_OK)
    return FailedToExecuteInit(stderr, orc, path);
}

static inline bool OnGraphSubmitted(const char* p, const void* event, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  if (!ComputeExecutionSchedule(orc->graph, kPriorityWeightedKahnScheduling)) {
    orc->run.success = false;
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(kStatusInvalidSpec));
    goto finished;
  }

  if (IsResourceGraphEmpty(orc->graph)) {
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(kStatusOk));
    goto finished;
  }

  const size_t num_resources = GetNumberOfResourcesInResourceGraph(orc->graph) + 1;
  InitPlan(&orc->plan, num_resources);
  InitActionLog(&orc->actions, num_resources);
  InitDeltaLog(&orc->dlog, num_resources);
  InitValidationLog(&orc->vlog, num_resources);

  DispatchReadyResources(orc);
finished:
  return true;
}

OrchestratorHandle NewOrchestrator(OrchestratorConfig config) {
  if (!config.root)
    return NULL;

  Orchestrator* orc = (Orchestrator*)malloc(sizeof(Orchestrator));
  if (orc) {
    memset(orc, 0, sizeof(Orchestrator));
    orc->config.root = strdup(config.root);
    orc->config.state_dir = strdup(config.state_dir);
    orc->config.cache_dir = strdup(config.cache_dir);
    InitRunInfoWithNewId(&orc->run, kOrchestratorPlanMode);
    orc->loop = uv_default_loop();
    orc->graph = NewResourceGraph();
    BootstrapHyphaCoreResources(orc->graph);
    orc->discovered_manifests = NULL;
    orc->num_discovered_manifests = 0;
#ifdef HYPHA_ENABLE_PROFILING
    uv_check_init(orc->loop, &orc->profiling_check);
    orc->profiling_check.data = orc;
    uv_check_start(&orc->profiling_check, &OnProfilingCheck);
#endif  // HYPHA_ENABLE_PROFILING

    InitOrcState(orc);
    InitOrcHistory(orc);
    orc->bus = (EventBus*)malloc(sizeof(EventBus));
    InitEventBus(orc->loop, orc->bus);
    InitOrcLuaState(orc);

    OrchestratorSubscribe(orc, GRAPH_SUBMITTED_EVENT, &OnGraphSubmitted, orc, NULL);
    OrchestratorSubscribe(orc, RECONCILE_COMPLETE_EVENT, &OnReconcileComplete, orc, NULL);
    OrchestratorSubscribe(orc, RECONCILE_COMPLETE_EVENT, &StopLoopOnReconcileDone, orc, NULL);
    OrchestratorSubscribe(orc, RECONCILE_FAILED_EVENT, &StopLoopOnReconcileDone, orc, NULL);

    ExecInit(orc);
    DiscoverManifestPaths(orc->L, &orc->discovered_manifests, &orc->num_discovered_manifests);
  }

  OrchestratorPublish(orc, ORCHESTRATOR_INIT_EVENT, NewOrchestratorInitEvent());
  return (OrchestratorHandle)orc;
}
