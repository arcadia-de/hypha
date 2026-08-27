#include "hypha/orchestrator.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <xxhash.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller.h"
#include "hypha/discovery.h"
#include "hypha/event.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/resource_decorator.h"
#include "hypha/resource_graph.h"
#include "hypha/run_info.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"
#include "orc.h"
#include "reconcile.h"

#define __DEFINE_GETTER(Type, Name, Value)        \
  Type* GetOrc##Name(OrchestratorHandle handle) { \
    return handle ? (Value) : NULL;               \
  }

#define _DEFINE_GETTER(Type, Value) __DEFINE_GETTER(Type, Type, Value);

#define DEFINE_GETTER(Type, Field)  _DEFINE_GETTER(Type, &((Orchestrator*)handle)->Field)

_DEFINE_GETTER(ResourceGraph, ((Orchestrator*)handle)->graph);
_DEFINE_GETTER(HistoryLog, ((Orchestrator*)handle)->history);
_DEFINE_GETTER(EventBus, ((Orchestrator*)handle)->bus);
__DEFINE_GETTER(const char, ConfigDir, ((Orchestrator*)handle)->config.root);
__DEFINE_GETTER(const char, StateDir, ((Orchestrator*)handle)->config.state_dir);
__DEFINE_GETTER(const char, CacheDir, ((Orchestrator*)handle)->config.cache_dir);
DEFINE_GETTER(ValidationLog, vlog);
DEFINE_GETTER(ActionLog, actions);
__DEFINE_GETTER(lua_State, LuaState, ((Orchestrator*)handle)->L);
DEFINE_GETTER(Plan, plan);
_DEFINE_GETTER(StateStore, ((Orchestrator*)handle)->state);

#undef DEFINE_GETTER
#undef _DEFINE_GETTER
#undef __DEFINE_GETTER

void GetOrchestratorMetrics(OrchestratorHandle handle, OrchestratorMetrics* out) {
  Orchestrator* orc = (Orchestrator*)handle;
  ASSERT(orc);
  memcpy(out, &orc->metrics, sizeof(OrchestratorMetrics));
}

void OrchestratorSubscribe(OrchestratorHandle handle, const char* p, EventCallbackFn cb, void* data,
                           void (*free_data)(void*)) {
  Orchestrator* orc = (Orchestrator*)handle;
  EventBusSubscribe(orc->bus, p, cb, data, free_data);
}

void OrchestratorPublish(OrchestratorHandle handle, const char* p, void* event) {
  Orchestrator* orc = (Orchestrator*)handle;
  EventBusPublish(orc->bus, p, event);
}

OrchestratorRunMode GetReconcileTaskRunMode(ReconcileTask* rhs) {
  ASSERT(rhs);
  return rhs->orc->run.mode;
}

bool OrchestratorCompact(OrchestratorHandle handle) {
  bool success = false;
  if (!handle)
    goto finished;

  Orchestrator* orc = (Orchestrator*)handle;
  success = StateStoreCompact(orc->state);
finished:
  return success;
}

bool OrchestratorPruneOrphans(OrchestratorHandle handle) {
  bool success = false;
  if (!handle)
    goto finished;

  LOG_INFO("OrchestratorPruneOrphans not implemented");
finished:
  return success;
}

void OrchestratorPrintRuntimeInfo(OrchestratorHandle handle) {
  Orchestrator* orc = (Orchestrator*)handle;
  ASSERT(orc);
  LOG_INFO("Hypha Runtime Info:");
  LOG_INFO("  config dir: %s", orc->config.root);
  LOG_INFO("  cache dir: %s", orc->config.cache_dir);
  LOG_INFO("  state dir: %s", orc->config.state_dir);
  LOG_INFO("  lua version: %.1f", lua_version(orc->L));
  LOG_INFO("  libuv version: %s", uv_version_string());
  LOG_INFO("  registered controllers:");
  for (int i = 0; i < GetNumberOfRegisteredControllers(); i++) {
    Controller* ctrl = GetControllerAt(i);
    ASSERT(ctrl);
    LOG_INFO("    - %s (%p)", GetControllerKind(ctrl), (void*)ctrl);
  }
  LOG_INFO("");
}

bool OrchestratorRunWithReason(OrchestratorHandle handle, const OrchestratorRunMode mode, const Reason reason) {
  bool success = false;
  if (!handle)
    goto finished;

  Orchestrator* orc = (Orchestrator*)handle;
  OrchestratorPublish(orc, GRAPH_SUBMITTED_EVENT, NewGraphSubmittedEvent());

  uuid_t id;
  uuid_generate_random(id);
  InitRunInfoWithReason(&orc->run, mode, id, reason);

  RunInfoStart(&orc->run);
  uv_run(orc->loop, UV_RUN_DEFAULT);
  success = RunInfoFinish(&orc->run);
  // TODO(@s0cks):
  //  SortPlan(&orc->plan, &DefaultPlannedActionComparator);
  //  SortActionLog(&orc->actions);
  //  SortValidationLog(&orc->vlog, &DefaultPlannedActionComparator);

finished:
  return success;
}

void FreeOrchestrator(OrchestratorHandle handle) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  if (orc->graph)
    FreeResourceGraph(orc->graph);

  FreeValidationLog(&orc->vlog, 0);
  FreeEventBus(orc->bus);
  StateStoreClose(orc->state);
  HistoryLogClose(orc->history);
  if (orc->L)
    lua_close(orc->L);

  free(orc);
}

void VisitDiscoveredManifests(OrchestratorHandle handle, VisitDiscoveredManifestFn fn, void* data) {
  Orchestrator* orc = (Orchestrator*)handle;
  for (size_t i = 0; i < orc->num_discovered_manifests; i++) {
    DiscoveredManifest* dm = &orc->discovered_manifests[i];
    if (!fn((uint64_t)i, dm, data))
      return;
  }
}
