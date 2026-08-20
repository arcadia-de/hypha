#include "hypha/orchestrator.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <xxhash.h>

#include "hypha.h"
#include "hypha/controller.h"
#include "hypha/discovery.h"
#include "hypha/event.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/planner.h"
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
  Plan plan;
  OrchestratorMetrics metrics;

  AppliedAction* actions;
  size_t actions_len;
  size_t actions_cap;

  size_t num_discovered_manifests;
  DiscoveredManifest* discovered_manifests;
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
  const int result = luaL_dofile(orc->L, path);  // TODO(@s0cks): check result
  if (result != LUA_OK)
    return FailedToExecuteInit(stderr, orc, path);
}

static void DispatchReadyResources(Orchestrator* orc);
static inline bool OnGraphSubmitted(const char* p, const void* event, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  if (!ComputeExecutionSchedule(orc->graph, kDepthFirstScheduling)) {
    orc->failed = true;
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(kStatusInvalidSpec));
    goto finished;
  }

  fprintf(stdout, "order: ");
  for (ResourceGraphIndex i = 0; i < GetNumberOfResourcesInResourceGraph(orc->graph); i++) {
    fprintf(stdout, "%lu", ResourceGraphGetAtOrderIndex(orc->graph, i));
  }
  fprintf(stdout, "\n");

  if (IsResourceGraphEmpty(orc->graph)) {
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(kStatusOk));
    goto finished;
  }

  const size_t num_resources = GetNumberOfResourcesInResourceGraph(orc->graph);
  if (orc->run.mode == kOrchestratorPlanMode) {
    InitPlan(&orc->plan, num_resources);
  } else if (orc->run.mode == kOrchestratorApplyMode) {
    const size_t total_size = sizeof(AppliedAction) * (num_resources + 1);
    orc->actions = (AppliedAction*)malloc(total_size);
    orc->actions_len = 0;
    orc->actions_cap = num_resources;
  }

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
#ifdef HYPHA_ENABLE_PROFILING
  TracyCFrameMark;
#endif  // HYPHA_ENABLE_PROFILING

  return (OrchestratorHandle)orc;
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

static inline int CompareAppliedAction(const void* lhs, const void* rhs) {
  const AppliedAction* a = (const AppliedAction*)lhs;
  const AppliedAction* b = (const AppliedAction*)rhs;
  if (a->action < b->action) {
    return -1;
  } else if (a->action > b->action) {
    return +1;
  }

  const struct timespec* lhst = &a->timestamp;
  const struct timespec* rhst = &b->timestamp;
  if (lhst->tv_sec < rhst->tv_sec)
    return -1;
  else if (lhst->tv_sec > rhst->tv_sec)
    return +1;

  if (lhst->tv_nsec < rhst->tv_nsec)
    return -1;
  else if (lhst->tv_nsec > rhst->tv_nsec)
    return +1;
  return 0;
}

bool OrchestratorRunWithReason(OrchestratorHandle handle, const OrchestratorRunMode mode, const Reason reason) {
  bool success = false;
  if (!handle)
    goto finished;

  Orchestrator* orc = (Orchestrator*)handle;
  orc->run.mode = mode;
  memcpy(orc->run.reason, reason, sizeof(Reason));

  OrchestratorPublish(orc, GRAPH_SUBMITTED_EVENT, NewGraphSubmittedEvent());

  orc->metrics.run_start = uv_hrtime();
  uv_run(orc->loop, UV_RUN_DEFAULT);
  orc->metrics.run_finished = uv_hrtime();
  success = !orc->failed;

  qsort(orc->actions, orc->actions_len, sizeof(AppliedAction), &CompareAppliedAction);

finished:
  return success;
}

Plan* GetOrchestratorPlan(OrchestratorHandle handle) {
  return handle ? &((Orchestrator*)handle)->plan : NULL;
}

void FreeOrchestrator(OrchestratorHandle handle) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  if (orc->graph)
    FreeResourceGraph(orc->graph);

  FreeEventBus(orc->bus);
  StateStoreClose(orc->state);
  if (orc->L)
    lua_close(orc->L);

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

void OrchestratorAddResource(OrchestratorHandle handle, Resource* res) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  Resource* new_res = AllocNewResouceInGraph(orc->graph);
  ResourceInfo* source_info = &res->info;
  ResourceInfo* dest_info = &new_res->info;

  new_res->id = strdup(res->id);
  if (!new_res->id)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  new_res->kind = strdup(res->kind);
  if (!new_res->kind)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  if (res->spec.raw) {
    new_res->spec.raw = strdup(res->spec.raw);
    if (!new_res->spec.raw)
      return;  // TODO(@s0cks): probably should reclaim the allocated id
    new_res->spec.doc = NULL;
    new_res->spec.hash = XXH3_64bits(new_res->spec.raw, strlen(new_res->spec.raw));
  }

  if (source_info->labels_len > 0) {
    const size_t total_size = sizeof(Label) * source_info->labels_len;
    Label* new_labels = (Label*)malloc(total_size);
    LOG_FATAL_IF(!new_labels, "failed to allocate new labels for new resource");
    memset(new_labels, 0, total_size);
    memcpy(new_labels, source_info->labels, total_size);

    dest_info->labels = new_labels;
    dest_info->labels_len = dest_info->labels_cap = source_info->labels_len;
  } else {
    dest_info->labels = NULL;
    dest_info->labels_cap = dest_info->labels_len = 0;
  }

  if (source_info->annotations_len > 0) {
    const size_t total_size = sizeof(Annotation) * source_info->annotations_len;
    Annotation* new_annotations = (Annotation*)malloc(total_size);
    LOG_FATAL_IF(!new_annotations, "failed to allocate new annotations for new resource");
    memset(new_annotations, 0, total_size);
    memcpy(new_annotations, source_info->annotations, total_size);

    dest_info->annotations = new_annotations;
    dest_info->annotations_len = dest_info->annotations_cap = source_info->annotations_len;
  } else {
    dest_info->annotations = NULL;
    dest_info->annotations_cap = dest_info->annotations_len = 0;
  }

  new_res->state = kResourcePending;
  new_res->num_depends_on = res->num_depends_on;

  if (res->num_depends_on > 0) {
    char** depends_on = (char**)malloc(sizeof(char*) * res->num_depends_on);
    if (!depends_on)
      return;  // TODO(@s0cks): probably should reclaim the allocated id

    for (int i = 0; i < res->num_depends_on; i++)
      depends_on[i] = strdup(res->depends_on[i]);

    new_res->depends_on = depends_on;
  } else {
    new_res->depends_on = NULL;  // NOLINT(modernize-use-nullptr)
  }
}

static inline bool Validate(Controller* ctrl, const Resource* desired) {
  Reason reason;
  memset(reason, '\0', sizeof(reason));
  const ControllerValidationResult result = ControllerValidate(ctrl, desired, &reason);
  LOG_ERROR_IF(result == kValidationkFailed, "validation failed: %s", reason);
  return result != kValidationkFailed;
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

  task->action = ControllerPlan(ctrl, observed, desired, &task->reason);
  if (IsApplyReconcileTask(task))
    task->status = ControllerApply(ctrl, desired, task->action);

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
      .hash = res->spec.hash,
      .observed_json = res->spec.raw ? res->spec.raw : "",
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

  if (IsPlanReconcileTask(task)) {
    PlannedAction action;
    memset(&action, 0, sizeof(PlannedAction));
    action.id = strdup(res->id);
    memcpy(action.reason, task->reason, sizeof(Reason));
    action.action = task->action;
    AppendPlannedAction(&orc->plan, &action);
  } else if (IsApplyReconcileTask(task)) {
    AppliedAction* action = NewAppliedAction(orc);
    action->id = strdup(res->id);
    memcpy(action->reason, task->reason, sizeof(Reason));
    action->action = task->action;
    memcpy(&action->timestamp, &task->start, sizeof(struct timespec));
  }

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
  clock_gettime(CLOCK_REALTIME, &task->start);
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

OrchestratorRunMode GetReconcileTaskRunMode(ReconcileTask* rhs) {
  ASSERT(rhs);
  return rhs->orc->run.mode;
}

void VisitDiscoveredManifests(OrchestratorHandle handle, VisitDiscoveredManifestFn fn, void* data) {
  Orchestrator* orc = (Orchestrator*)handle;
  for (size_t i = 0; i < orc->num_discovered_manifests; i++) {
    DiscoveredManifest* dm = &orc->discovered_manifests[i];
    if (!fn((uint64_t)i, dm, data))
      return;
  }
}

AppliedAction* NewAppliedAction(OrchestratorHandle handle) {
  Orchestrator* orc = (Orchestrator*)handle;
  if (orc->actions_len + 1 > orc->actions_cap) {
    size_t new_cap = (orc->actions_cap + 1) * 2;  // TODO(@s0cks): round up pow2
    size_t total_size = sizeof(AppliedAction) * new_cap;
    AppliedAction* new_actions = (AppliedAction*)realloc(orc->actions, total_size);
    if (!new_actions)
      return NULL;

    orc->actions = new_actions;
    orc->actions_cap = new_cap;
  }

  AppliedAction* new_action = &orc->actions[orc->actions_len];
  orc->actions_len++;
  memset(new_action, 0, sizeof(AppliedAction));
  return new_action;
}

void VisitAppliedActions(OrchestratorHandle handle, VisitAppliedActionFn fn, void* data) {
  Orchestrator* orc = (Orchestrator*)handle;
  for (size_t i = 0; i < orc->actions_len; i++) {
    if (!fn((uint64_t)i, &orc->actions[i], data))
      return;
  }
}
