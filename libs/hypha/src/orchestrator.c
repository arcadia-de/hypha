#include "hypha/orchestrator.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>

#include "event.h"
#include "event_bus.h"
#include "hypha.h"
#include "hypha/controller.h"
#include "hypha/event.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/resource_graph.h"
#include "hypha/state.h"
#include "reconcile.h"

#ifndef UV_OK
#define UV_OK 0
#endif  // UV_OK

struct _Orchestrator {
  OrchestratorConfig config;

  uv_loop_t* loop;
  ResourceGraph* graph;
  EventBus events;
  lua_State* L;
  uint32_t pending;
  bool failed;
  StateStore* state;
  HistoryLog* history;

  uint64_t run_time;
  char* run_reason;
};

lua_State* NewOrchestratorLuaState(Orchestrator* orc);

static void DispatchReadyResources(Orchestrator* orc);

OrchestratorHandle NewOrchestrator(OrchestratorConfig config) {
  if (!config.root)
    return NULL;

  Orchestrator* orc = (Orchestrator*)malloc(sizeof(Orchestrator));
  if (orc) {
    memset(orc, 0, sizeof(Orchestrator));
    orc->config.root = strdup(config.root);
    orc->config.state_dir = strdup(config.state_dir);
    orc->config.cache_dir = strdup(config.cache_dir);
    orc->loop = uv_default_loop();
    orc->graph = NewResourceGraph();

    {
      char p[PATH_MAX];
      snprintf(p, PATH_MAX, "%s/journal.log", orc->config.state_dir);
      orc->state = StateStoreOpen(p);
      if (!orc->state) {
        LOG_FATAL("failed to open state store: %s", p);
        return NULL;
      }
    }
    ASSERT(orc->state);

    {
      char p[PATH_MAX];
      snprintf(p, PATH_MAX, "%s/history.log", orc->config.state_dir);
      orc->history = HistoryLogOpen(p, 5 * 1024 * 1024, 2);
      if (!orc->history) {
        LOG_FATAL("failed to open history log: %s", p);
        return NULL;
      }
    }
    ASSERT(orc->history);

    orc->run_time = 0;
    orc->run_reason = NULL;

    InitEventBus(orc->loop, &orc->events);

    lua_State* L = NewOrchestratorLuaState(orc);
    if (!L) {
      LOG_FATAL("failed to create orchestrator lua state");
      return NULL;  // TODO(@s0cks): cleanup resources
    }
    orc->L = L;
  }

  return (OrchestratorHandle)orc;
}

static inline void OnGraphSubmitted(OrchestratorHandle handle, const OrchestratorEvent* event, void* data) {
  ASSERT(handle);
  Orchestrator* orc = (Orchestrator*)handle;
  if (!ComputeExecutionSchedule(orc->graph)) {
    orc->failed = true;
    const OrchestratorEvent failed = {
        .kind = kReconcileFailedEvent,
        .status = kStatusInvalidSpec,
    };
    OrchestratorPublish(orc, &failed);
    return;
  }

  if (IsResourceGraphEmpty(orc->graph)) {
    const OrchestratorEvent done = {.kind = kReconcileCompleteEvent, .status = kStatusOk};
    OrchestratorPublish(orc, &done);
    return;
  }

  DispatchReadyResources(orc);
}

static inline void StopLoopOnReconcileDone(OrchestratorHandle handle, const OrchestratorEvent* event, void* data) {
  ASSERT(handle);
  ASSERT(event);

  Orchestrator* orc = (Orchestrator*)handle;
  uv_stop(orc->loop);
}

static inline void OnReconcileComplete(OrchestratorHandle handle, const OrchestratorEvent* event, void* data) {
  LOG_INFO("reconcile complete");
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

  DLOG_DEBUG("executing init file %s....", path);
  const int result = luaL_dofile(orc->L, path);  // TODO(@s0cks): check result
  if (result != LUA_OK)
    return FailedToExecuteInit(stderr, orc, path);
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

bool OrchestratorRun(OrchestratorHandle handle) {
  bool success = false;
  if (!handle)
    goto finished;

  Orchestrator* orc = (Orchestrator*)handle;
  OrchestratorPrintRuntimeInfo(handle);

  OrchestratorOnGraphSubmitted(handle, &OnGraphSubmitted, NULL);
  OrchestratorOnReconcileComplete(handle, &OnReconcileComplete, NULL);
  OrchestratorOnReconcileComplete(handle, &StopLoopOnReconcileDone, NULL);
  OrchestratorOnReconcileFailed(handle, &StopLoopOnReconcileDone, NULL);

  ExecInit(orc);
  OrchestratorPublishGraphSubmitted(handle);

  orc->run_time = 0;
  orc->run_reason = NULL;

  // orc->run_time = (uint64_t)uv_now(orc->loop);
  // orc->run_reason = NULL;

  uv_run(orc->loop, UV_RUN_DEFAULT);
  success = !orc->failed;
  if (!success)
    goto finished;
finished:
  return success;
}

void FreeOrchestrator(OrchestratorHandle handle) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  if (orc->graph)
    FreeResourceGraph(orc->graph);

  FreeEventBus(&orc->events);
  lua_close(orc->L);

  if (orc->state)
    StateStoreClose(orc->state);

  free(orc);
}

ResourceGraph* OrchestratorGetResourceGraph(OrchestratorHandle handle) {
  if (!handle)
    return NULL;
  return ((Orchestrator*)handle)->graph;
}

HistoryLog* OrchestratorGetHistoryLog(OrchestratorHandle handle) {
  if (!handle)
    return NULL;
  return ((Orchestrator*)handle)->history;
}

lua_State* OrchestratorGetLuaState(OrchestratorHandle handle) {
  return handle ? ((Orchestrator*)handle)->L : NULL;
}

EventBus* OrchestratorGetEventBus(OrchestratorHandle handle) {
  return handle ? &((Orchestrator*)handle)->events : NULL;
}

OrchestratorHandle EventBusGetOrchestrator(EventBus* bus) {
  return bus ? container_of(bus, Orchestrator, events) : NULL;
}

void OrchestratorAddResource(OrchestratorHandle handle, Resource res) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  Resource* new_res = AllocNewResouceInGraph(orc->graph);

  new_res->id = strdup(res.id);
  if (!new_res->id)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  new_res->kind = strdup(res.kind);
  if (!new_res->kind)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  new_res->spec = strdup(res.spec);
  if (!new_res->spec)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  new_res->state = kResourcePending;
  new_res->num_depends_on = res.num_depends_on;

  if (res.num_depends_on > 0) {
    char** depends_on = (char**)malloc(sizeof(char*) * res.num_depends_on);
    if (!depends_on)
      return;  // TODO(@s0cks): probably should reclaim the allocated id

    for (int i = 0; i < res.num_depends_on; i++)
      depends_on[i] = strdup(res.depends_on[i]);

    new_res->depends_on = depends_on;
  } else {
    new_res->depends_on = NULL;  // NOLINT(modernize-use-nullptr)
  }
}

static inline void ReconcileWork(uv_work_t* req) {
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Controller* ctrl = task->ctrl;
  Resource* observed = &task->observed;
  memset(observed, 0, sizeof(Resource));
  Resource* desired = GetResourceInGraph(task->orc->graph, task->index);

  SetLogResourceContext(desired->id, desired->kind);
  ControllerObserve(ctrl, desired, observed);
  task->action = ControllerPlan(ctrl, observed, desired);
  task->status = ControllerApply(ctrl, desired, task->action);
  ClearLogResourceContext();
}

static inline void ReconcileAfterWork(uv_work_t* req, int status) {
  // **Write after apply, not at end of run.**
  // In `ReconcileAfterWork` (loop thread), once a resource lands on `Ready`, `StateStorePut` its entry — `kind`,
  // `spec_hash` (hash the desired spec so a future run can tell "changed" from "unchanged" without holding the full
  // spec in memory), `observed_json` from `observe_after`, `applied_at`, `last_status`. This is unchanged from the
  // original design — the point of write-through-per-resource rather than batched-at-end is exactly what makes a killed
  // mid-run process leave honest state behind.

  ASSERT(req);
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Orchestrator* orc = task->orc;
  Resource* res = GetResourceInGraph(orc->graph, task->index);
  SetLogResourceContext(res->id, res->kind);

  if (status != 0) {
    res->state = kResourceFailed;
    orc->failed = true;
  } else {
    res->state = (task->status == kStatusOk) ? kResourceReady : kResourceFailed;
    if (res->state == kResourceFailed)
      orc->failed = true;
  }

  StateEntry entry = {
      .id = strdup(res->id),
      .kind = strdup(res->kind),
      .applied_at = (int64_t)orc->run_time,
      .last_status = res->state,
      .orphaned = false,
      .hash = "12739018201",
      .observed_json = "{}",
  };
  if (!StateStorePut(orc->state, &entry))
    LOG_ERROR("failed to write state entry for %s", res->id);

  HistoryRecord record = {
      .id = strdup(res->id),
      .kind = strdup(res->kind),
      .action = task->action,
      .status = task->status,
      // uint64_t run_id;
      // char* hash_before;
      // char* hash_after;
      .reason = orc->run_reason ? strdup(orc->run_reason) : NULL,
      .applied_at = (int64_t)orc->run_time,
  };
  if (!HistoryLogAppend(orc->history, &record))
    LOG_ERROR("failed to write to history for: %s", res->id);

  const OrchestratorEvent event = {
      .kind = kResourceDecoratedEvent,
      .resource = res,
      .action = task->action,
      .status = task->status,
  };
  OrchestratorPublish(orc, &event);

  free(task);
  orc->pending--;
  DispatchReadyResources(orc);
  ClearLogResourceContext();
}

static inline void MaybeFinishReconciliation(Orchestrator* orc) {
  ASSERT(orc);
  if (orc->pending > 0)
    return;

  ResourceGraph* graph = orc->graph;
  for (uint32_t i = 0; i < GetNumberOfResourcesInResourceGraph(graph); i++) {
    Resource* res = GetResourceInGraph(graph, i);
    if (IsResourcePending(res)) {
      LOG_ERROR("resource %s unreachable: upstream dependency failed", res->id);
      res->state = kResourceFailed;
      orc->failed = true;
    }
  }

  if (!StateStoreFlush(orc->state))
    LOG_ERROR("failed to flush state store");

  const OrchestratorEvent done = {
      .kind = orc->failed ? kReconcileFailedEvent : kReconcileCompleteEvent,
      .status = orc->failed ? kStatusInternalError : kStatusOk,
  };
  OrchestratorPublish(orc, &done);
}

static inline void DispatchReadyResources(Orchestrator* orc) {
  ASSERT(orc);
  ResourceGraph* graph = orc->graph;

  for (uint32_t i = 0; i < GetNumberOfResourcesInResourceGraph(graph); i++) {
    Resource* res = GetResourceInGraph(graph, i);
    if (!IsResourcePending(res))
      continue;

    if (!DependenciesAreSatisfied(graph, res))
      continue;

    Controller* ctrl = GetControllerForKind(res->kind);
    if (!ctrl) {
      LOG_ERROR("no controller registered for kind '%s' (resource: %s)", res->kind, res->id);
      res->state = kResourceFailed;
      orc->failed = true;
      continue;
    }

    res->state = kResourceProcessing;
    orc->pending++;

    ReconcileTask* task = (ReconcileTask*)malloc(sizeof(ReconcileTask));
    memset(task, 0, sizeof(ReconcileTask));
    if (!StateStoreGet(orc->state, res->id, &task->last_applied)) {
      LOG_WARN("failed to get last state store value for: %s (%s)", res->id, res->kind);
    }

    task->orc = orc;
    task->index = (ResourceGraphIndex)i;
    task->ctrl = ctrl;
    task->action = kNoAction;
    task->status = kStatusOk;
    memset(&task->observed, 0, sizeof(Resource));

    uv_queue_work(orc->loop, &task->work, ReconcileWork, ReconcileAfterWork);
  }

  MaybeFinishReconciliation(orc);
}

void OrchestratorRenderResourceGraphTo(OrchestratorHandle handle, const char* name, const char* layout,
                                       const char* render, FILE* stream) {
  ASSERT(handle);
  ASSERT(name);
  ASSERT(layout);
  ASSERT(render);
  ASSERT(stream);
  Orchestrator* orc = (Orchestrator*)handle;
  return RenderResourceGraphToGraphvizWithLayout(orc->graph, name, layout, render, stream);
}
