#include "hypha/orchestrator.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>

#include "hypha.h"
#include "hypha/controller.h"
#include "hypha/event.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/resource_graph.h"
#include "hypha/state.h"
#include "reconcile.h"

#ifdef HYPHA_ENABLE_PROFILING
#include <tracy/tracy/TracyC.h>
#endif  // HYPHA_ENABLE_PROFILING

#ifndef UV_OK
#define UV_OK 0
#endif  // UV_OK

struct _Orchestrator {
  OrchestratorConfig config;
  OrchestratorRunConfig run;

  uv_loop_t* loop;
#ifdef HYPHA_ENABLE_PROFILING
  uv_check_t profiling_check;
#endif  // HYPHA_ENABLE_PROFILING

  ResourceGraph* graph;
  EventBus* bus;
  lua_State* L;
  uint32_t pending;
  bool failed;
  StateStore* state;
  HistoryLog* history;

  PlannedAction* actions;

  OrchestratorMetrics metrics;
};

#ifdef HYPHA_ENABLE_PROFILING

static inline void OnProfilingCheck(uv_check_t* handle) {
  TracyCFrameMark;
}

#endif  // HYPHA_ENABLE_PROFILING

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

  DLOG_DEBUG("executing init file %s....", path);
  const int result = luaL_dofile(orc->L, path);  // TODO(@s0cks): check result
  if (result != LUA_OK)
    return FailedToExecuteInit(stderr, orc, path);
}

static void DispatchReadyResources(Orchestrator* orc);
static inline bool OnGraphSubmitted(const char* p, const void* event, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  if (!ComputeExecutionSchedule(orc->graph)) {
    orc->failed = true;
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(kStatusInvalidSpec));
    goto finished;
  }

  if (IsResourceGraphEmpty(orc->graph)) {
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(kStatusOk));
    goto finished;
  }

  if (orc->run.mode == kOrchestratorPlanMode)
    orc->actions = (PlannedAction*)malloc(sizeof(PlannedAction) * GetNumberOfResourcesInResourceGraph(orc->graph));

  DispatchReadyResources(orc);
finished:
  return true;
}

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

static inline void DiscoverManifestPaths(Orchestrator* orc, char*** results, size_t* num_results) {
#define L orc->L
  size_t capacity = 0;

  char** values = NULL;
  size_t num_values = 0;
  const int stack_size = lua_gettop(L);
  if (stack_size > 0) {
    const int stack_size = lua_gettop(L);
    if (stack_size > 0) {
      if (!lua_isnil(L, -1) && !lua_istable(L, -1)) {
        LOG_ERROR("expected the config to return nil or a table, received: %s", lua_typename(L, lua_type(L, -1)));
      } else if (lua_istable(L, -1)) {
        const size_t len = lua_rawlen(L, -1);

        if (len > 0) {
          capacity = len;
          values = (char**)malloc(sizeof(char*) * capacity);
          memset(values, 0, sizeof(char*) * capacity);
        }

        for (size_t i = 1; i <= len; i++) {
          lua_rawgeti(L, -1, (lua_Integer)i);

          if (lua_istable(L, -1)) {
            const size_t sub_len = lua_rawlen(L, -1);

            if (num_values + sub_len > capacity) {
              capacity = num_values + sub_len;
              values = (char**)realloc(values, sizeof(char*) * capacity);
            }

            for (size_t j = 1; j <= sub_len; j++) {
              lua_rawgeti(L, -1, (lua_Integer)j);

              if (!lua_isstring(L, -1)) {
                DLOG_WARN("expected subtable value at index %zu to be a string", j);
                lua_pop(L, 1);
                continue;
              }

              values[num_values] = strdup(lua_tostring(L, -1));
              num_values++;
              lua_pop(L, 1);
            }
            lua_pop(L, 1);

          } else if (lua_isstring(L, -1)) {
            if (num_values >= capacity) {
              capacity = num_values + 1;
              values = (char**)realloc(values, sizeof(char*) * capacity);
            }

            values[num_values] = strdup(lua_tostring(L, -1));
            num_values++;
            lua_pop(L, 1);

          } else {
            DLOG_WARN("expected table value at %zu to be a string or table", i);
            lua_pop(L, 1);
          }
        }
      }
    }
  }
#undef L
  (*results) = values;
  (*num_results) = num_values;
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
    orc->loop = uv_default_loop();
    orc->graph = NewResourceGraph();
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

#ifdef HYPHA_DEBUG
    OrchestratorPrintRuntimeInfo(orc);
#endif  // HYPHA_DEBUG

    ExecInit(orc);

    char** values = NULL;
    size_t num_values = 0;
    DiscoverManifestPaths(orc, &values, &num_values);
    if (values && num_values > 0) {
      DLOG_INFO("values (%zu):", num_values);
      for (size_t i = 0; i < num_values; i++)
        DLOG_INFO(" - %s", values[i]);
    }
  }

  OrchestratorPublish(orc, ORCHESTRATOR_INIT_EVENT, NewOrchestratorInitEvent());
#ifdef HYPHA_ENABLE_PROFILING
  TracyCFrameMark;
#endif  // HYPHA_ENABLE_PROFILING

  return (OrchestratorHandle)orc;
}

// TODO(@s0cks): convert to lua
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

bool OrchestratorRunWithReason(OrchestratorHandle handle, const OrchestratorRunMode mode, const Reason reason) {
  bool success = false;
  if (!handle)
    goto finished;

  Orchestrator* orc = (Orchestrator*)handle;
  orc->run.mode = mode;
  memcpy(orc->run.reason, reason, sizeof(Reason));

  OrchestratorPublish(orc, GRAPH_SUBMITTED_EVENT, NewGraphSubmittedEvent());

  LOG_DEBUG("running orchestrator loop...");
  orc->metrics.run_start = uv_hrtime();
  uv_run(orc->loop, UV_RUN_DEFAULT);
  orc->metrics.run_finished = uv_hrtime();
  LOG_DEBUG("orchestrator loop finished");
  success = !orc->failed;

finished:
  return success;
}

void OrchestratorVisitPlannedActions(OrchestratorHandle handle, PlannedActionVisitorFn fn, void* data) {
  Orchestrator* orc = (Orchestrator*)handle;
  if (!orc || !orc->actions)
    return;

  const size_t num_actions = GetNumberOfResourcesInResourceGraph(orc->graph);
  for (size_t i = 0; i < num_actions; i++) {
    PlannedAction* action = &orc->actions[i];
    if (!fn(action, data))
      return;
  }
}

void FreeOrchestrator(OrchestratorHandle handle) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  if (orc->graph)
    FreeResourceGraph(orc->graph);

  FreeEventBus(orc->bus);
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
  return handle ? ((Orchestrator*)handle)->bus : NULL;
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

  if (res.spec.raw) {
    new_res->spec.raw = strdup(res.spec.raw);
    if (!new_res->spec.raw)
      return;  // TODO(@s0cks): probably should reclaim the allocated id
    new_res->spec.doc = NULL;
  }

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

static inline bool Validate(Controller* ctrl, const Resource* desired) {
  Reason reason;
  memset(reason, '\0', sizeof(reason));
  const ControllerValidationResult result = ControllerValidate(ctrl, desired, reason);
  LOG_ERROR_IF(result == kValidationkFailed, "validation failed: %s", reason);
  return true;
}

static inline void ReconcileWork(uv_work_t* req) {
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Controller* ctrl = task->ctrl;
  Resource* observed = &task->observed;
  memset(observed, 0, sizeof(Resource));
  Resource* desired = GetResourceInGraph(task->orc->graph, task->index);

  if (desired->spec.raw) {
    json_error_t err;
    json_t* doc = json_loads(desired->spec.raw, 0, &err);
    if (!doc) {
      LOG_ERROR("invalid spec doc:\n%s", desired->spec.raw);
      LOG_ERROR("error on line %d: %s", err.line, err.text);
      return;
    }

    desired->spec.doc = doc;
  }

  SetLogResourceContext(desired->id, desired->kind);
  ControllerObserve(ctrl, desired, observed);

  if (!Validate(ctrl, desired))
    goto finished;

  task->action = ControllerPlan(ctrl, observed, desired, task->reason);
  switch (task->orc->run.mode) {
    case kOrchestratorPlanMode:
      PlannedAction* action = &task->orc->actions[task->index];
      action->id = desired->id;
      action->reason = task->reason;
      action->action = task->action;
      // do nothing
      break;
    case kOrchestratorApplyMode:
    default:
      task->status = ControllerApply(ctrl, desired, task->action);
  }

finished:
  ClearLogResourceContext();

  if (desired->spec.doc)
    json_decref(desired->spec.doc);
  desired->spec.doc = NULL;
}

static inline void WriteResourceState(Orchestrator* orc, const Resource* res) {
  StateEntry entry = {
      .id = strdup(res->id),
      .kind = strdup(res->kind),
      .applied_at = orc->metrics.run_start,
      .last_status = res->state,
      .orphaned = false,
      .hash = "",
      .observed_json = res->spec.raw,
  };
  LOG_ERROR_IF(!StateStorePut(orc->state, &entry), "failed to write state entry for %s", res->id);
}

static inline void WriteHistory(Orchestrator* orc, const Resource* res, const ControllerAction action,
                                const ControllerStatus status) {
  HistoryRecord record = {
      .id = strdup(res->id),
      .kind = strdup(res->kind),
      .action = action,
      .status = status,
      // uint64_t run_id;
      // char* hash_before;
      // char* hash_after;
      .applied_at = (int64_t)orc->metrics.run_finished,
  };
  memcpy(record.reason, orc->run.reason, sizeof(Reason));  // TODO(@s0cks): should check if reason is not empty first
  LOG_ERROR_IF(!HistoryLogAppend(orc->history, &record), "failed to write to history for: %s", res->id);
}

static inline void ReconcileAfterWork(uv_work_t* req, int status) {
  ASSERT(req);
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Orchestrator* orc = task->orc;
  Resource* res = GetResourceInGraph(orc->graph, task->index);

  SetLogResourceContext(res->id, res->kind);
  orc->metrics.num_actions[task->action]++;

  if (status != 0) {
    res->state = kResourceFailed;
    orc->failed = true;
  } else {
    res->state = (task->status == kStatusOk) ? kResourceReady : kResourceFailed;
    if (res->state == kResourceFailed)
      orc->failed = true;
  }

  WriteResourceState(orc, res);
  WriteHistory(orc, res, task->action, task->status);
  free(task);
  orc->pending--;
  orc->metrics.num_processed++;
  ClearLogResourceContext();

  DispatchReadyResources(orc);
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

  ControllerStatus status = kStatusInternalError;
  if (orc->failed) {
    status = kStatusInternalError;
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(status));
  } else {
    status = kStatusOk;
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(status));
  }

  OrchestratorPublish(orc, RECONCILE_FINISHED_EVENT, NewReconcileFinishedEvent(status));
}

static inline void QueueReconcileTask(Orchestrator* orc, Controller* ctrl, const ResourceGraphIndex index,
                                      Resource* res) {
  ReconcileTask* task = (ReconcileTask*)malloc(sizeof(ReconcileTask));
  memset(task, 0, sizeof(ReconcileTask));
  if (!StateStoreGet(orc->state, res->id, &task->last_applied)) {
    LOG_WARN("failed to get last state store value for: %s (%s)", res->id, res->kind);
  }

  task->orc = orc;
  task->index = index;
  task->ctrl = ctrl;
  task->action = kNoAction;
  task->status = kStatusOk;
  memset(&task->observed, 0, sizeof(Resource));

  uv_queue_work(orc->loop, &task->work, ReconcileWork, ReconcileAfterWork);
}

static inline void DispatchReadyResources(Orchestrator* orc) {
  ASSERT(orc);
  ResourceGraph* graph = orc->graph;

  for (size_t i = 0; i < GetNumberOfResourcesInResourceGraph(graph); i++) {
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
    QueueReconcileTask(orc, ctrl, i, res);
  }

  MaybeFinishReconciliation(orc);
}
