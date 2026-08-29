#include "reconcile.h"

#include <uuid.h>
#include <xxhash.h>

#include "hypha.h"
#include "hypha/annotation.h"
#include "hypha/controller_status.h"
#include "hypha/history.h"
#include "hypha/label.h"
#include "hypha/log.h"
#include "hypha/orchestrator.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/resource_graph.h"
#include "hypha/resource_kind.h"
#include "hypha/run_mode.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"

#define BEGIN_RESOURCE_LOG_CTX(Resource)                               \
  ({                                                                   \
    ResourceIdStr id;                                                  \
    ResourceIdCStr(&(Resource)->id, id);                               \
    SetLogResourceContext(id, FindResourceKindName((Resource)->kind)); \
  })

#define END_RESOURCE_LOG_CTX ClearLogResourceContext();

static inline bool CheckPending(ResourceGraphIndex idx, Resource* res, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  if (IsResourcePending(res)) {
    ResourceIdStr id_str;
    ResourceIdCStr(&res->id, id_str);
    LOG_ERROR("resource %s unreachable: upstream dependency failed", id_str);
    res->state = kResourceFailed;
    orc->run.status = kStatusInternalError;
  }

  return true;
}

static inline void MaybeFinishReconciliation(Orchestrator* orc) {
  ASSERT(orc);
  if (orc->pending > 0)
    return;

  ResourceGraph* graph = orc->graph;
  VisitAllResources(graph, CheckPending, orc);

  if (!StateStoreFlush(orc->state))
    LOG_ERROR("failed to flush state store");

  ControllerStatus status = kStatusInternalError;
  if (orc->run.status != kStatusOk) {
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(status));
  } else {
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(status));
  }

  OrchestratorPublish(orc, RECONCILE_FINISHED_EVENT, NewReconcileFinishedEvent(status));
}

#define CHECK_MODE(Name)                       \
  if (task->mode <= kOrchestrator##Name##Mode) \
    goto finished;

static inline void ReconcileWork(uv_work_t* req) {
  ASSERT(req);
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Controller* ctrl = task->ctrl;
  Resource* desired = GetResourceInGraph(task->orc->graph, task->index);
  ResourceSpecParseJson(&desired->spec);
  Resource* observed = &task->observed;

  BEGIN_RESOURCE_LOG_CTX(desired);
  const ControllerStatus status = ControllerObserve(ctrl, observed, &task->last);
  if (status != kStatusOk)
    goto finished;
  if (task->mode <= kOrchestratorObserveMode) {
    memcpy(desired, observed, sizeof(Resource));
    goto finished;
  }

  const Label* defaults = GetDefaultLabels();
  const size_t num_defaults = GetNumberOfDefaultLabels();
  if (defaults != NULL && num_defaults > 0)
    ResourcePushLabels(desired, defaults, num_defaults);
  task->action = ControllerPlan(ctrl, observed, desired, &task->plan);
  if (IsPlanReconcileTask(task) || task->action == kNoAction)
    goto finished;
  CHECK_MODE(Plan);

  if (!ControllerValidate(ctrl, desired, &task->vlog))
    goto finished;
  CHECK_MODE(Validate);

  switch (task->mode) {
    case kOrchestratorApplyMode:
      task->status = ControllerApply(ctrl, desired, task->action, &task->alog);
      break;
    case kOrchestratorDiffMode:
    case kOrchestratorDestroyMode:
    default:
      DLOG_WARN("task status is no-op because mode `%s` is set", OrchestratorRunModeName(task->mode));
      task->status = kStatusNoOp;
  }
finished:
  END_RESOURCE_LOG_CTX;
  FreeResourceSpecJson(&desired->spec);
  FreeResourceSpecJson(&observed->spec);
}

#undef CHECK_MODE

static inline void WriteResourceState(Orchestrator* orc, const Resource* res) {
  ASSERT(res->spec.raw);
  ResourceIdStr id_str;
  ResourceIdCStr(&res->id, id_str);

  const char* kind = FindResourceKindName(res->kind);
  StateEntry entry = {
      .id = strdup(id_str),
      .kind = strdup(kind),
      .name = res->info.name ? strdup(res->info.name) : NULL,
      .applied_at = orc->metrics.run_start,
      .last_status = res->state,
      .orphaned = false,
      .hash = res->spec.hash,
      .observed_json = res->spec.raw,
      .labels = res->info.labels,
      .labels_len = res->info.labels_len,
      .annotations = res->info.annotations,
      .annotations_len = res->info.annotations_len,
  };
  LOG_ERROR_IF(!StateStorePut(orc->state, &entry), "failed to write state entry for %s", id_str);
  DLOG_INFO("wrote %zu labels to state store", entry.labels_len);
  free(entry.id);
  free(entry.kind);
  free(entry.name);
}

static inline void WriteHistory(Orchestrator* orc, const Resource* res, const ReconcileTask* task) {
  ResourceIdStr id_str;
  ResourceIdCStr(&res->id, id_str);

  const char* kind = FindResourceKindName(res->kind);
  HistoryRecord record = {
      .id = strdup(id_str),
      .kind = strdup(kind),
      .name = res->info.name ? strdup(res->info.name) : NULL,
      .action = task->action,
      .status = task->status,
      .hash_before = task->observed.spec.hash,
      .hash_after = res->spec.hash,
      .applied_at = (int64_t)orc->metrics.run_finished,
      .labels = res->info.labels,
      .labels_len = res->info.labels_len,
      .annotations = res->info.annotations,
      .annotations_len = res->info.annotations_len,
  };
  uuid_copy(record.run_id, orc->run.id);
  memcpy(record.reason, task->reason, sizeof(Reason));  // TODO(@s0cks): should check if reason is not empty first
  LOG_ERROR_IF(!HistoryLogAppend(orc->history, &record), "failed to write to history for: %s", id_str);

  free(record.id);
  free(record.kind);
  free(record.name);
}

static inline ControllerStatus UpdateResourceState(const int status, const ReconcileTask* task, ResourceState* state) {
  if (status != 0) {
    DLOG_ERROR("invalid status: %d", status);
    (*state) = kResourceFailed;
    return false;
  }

  (*state) = (task->status == kStatusOk) ? kResourceReady : kResourceFailed;
  DLOG_ERROR_IF(task->status != kStatusOk, "task status is not ok");
  return task->status;
}

static inline void ReconcileAfterWork(uv_work_t* req, int status) {
  ASSERT(req);
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Orchestrator* orc = task->orc;
  Resource* res = GetResourceInGraph(orc->graph, task->index);

  BEGIN_RESOURCE_LOG_CTX(res);
  orc->metrics.num_actions[task->action]++;

  AppendPlan(&orc->plan, &task->plan);
  AppendValidationLog(&orc->vlog, &task->vlog);

  if (!IsDeltaLogEmpty(&task->dlog))
    AppendDeltaLog(&orc->dlog, &task->dlog);

  orc->run.status = UpdateResourceState(status, task, &res->state);
  if (IsApplyReconcileTask(task)) {
    if (!IsActionLogEmpty(&task->alog))
      AppendActionLog(&orc->actions, &task->alog);

    WriteResourceState(orc, res);
    if (task->action != kNoAction)
      WriteHistory(orc, res, task);
  }

  free(task);
  orc->pending--;
  orc->metrics.num_processed++;
  END_RESOURCE_LOG_CTX;
  DispatchReadyResources(orc);
}

static inline bool QueueReconcileTaskForResource(const ResourceGraphIndex idx, Resource* res, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  ResourceGraph* graph = orc->graph;
  if (!IsResourcePending(res))
    return true;

  if (!DependenciesAreSatisfied(orc->graph, res))
    return true;

  Controller* ctrl = GetControllerForKindName(FindResourceKindName(res->kind));
  if (!ctrl) {
    ResourceIdStr id_str;
    ResourceIdCStr(&res->id, id_str);
    LOG_ERROR("no controller registered for kind '%s' (resource: %s)", res->kind, id_str);
    res->state = kResourceFailed;
    orc->run.status = kStatusInternalError;
    return true;
  }

  res->state = kResourceProcessing;
  orc->pending++;
  QueueReconcileTask(orc, ctrl, idx, res);
  return true;
}

void DispatchReadyResources(Orchestrator* orc) {
  ASSERT(orc);
  ResourceGraph* graph = orc->graph;
  VisitAllResources(graph, QueueReconcileTaskForResource, orc);
  MaybeFinishReconciliation(orc);
}

void QueueReconcileTask(Orchestrator* orc, Controller* ctrl, const ResourceGraphIndex index, Resource* res) {
  static const size_t init_cap = 3;

  ReconcileTask* task = (ReconcileTask*)malloc(sizeof(ReconcileTask));
  memset(task, 0, sizeof(ReconcileTask));
  ResourceIdStr id_str;
  ResourceIdCStr(&res->id, id_str);

  task->orc = orc;
  clock_gettime(CLOCK_REALTIME, &task->start);
  task->mode = orc->run.mode;
  task->index = index;
  task->ctrl = ctrl;
  task->action = kNoAction;
  task->status = kStatusOk;
  InitPlan(&task->plan, init_cap);
  InitValidationLog(&task->vlog, init_cap);
  InitDeltaLog(&task->dlog, init_cap);
  InitActionLog(&task->alog, init_cap);

  memset(&task->observed, 0, sizeof(Resource));
  memset(&task->last, 0, sizeof(StateEntry));
  if (StateStoreGet(orc->state, id_str, &task->last)) {
    Resource* observed = &task->observed;
    uuid_parse(task->last.id, observed->id);
    observed->kind = FindResourceKind(task->last.kind);
    observed->state = (ResourceState)task->last.last_status;
    observed->spec.hash = task->last.hash;
    observed->info.name = strdup(task->last.name);
    observed->spec.raw = strdup(task->last.observed_json);
    ResourceSpecParseJson(&observed->spec);
  } else {
    DLOG_WARN("failed to decode value from state store");
  }

  uv_queue_work(orc->loop, &task->work, ReconcileWork, ReconcileAfterWork);
}
