#include "reconcile.h"

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/orchestrator.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/resource_graph.h"
#include "hypha/validation_log.h"
#include "orc.h"

static inline bool CheckPending(ResourceGraphIndex idx, Resource* res, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  if (IsResourcePending(res)) {
    ResourceIdStr id_str;
    ResourceIdCStr(&res->id, id_str);
    LOG_ERROR("resource %s unreachable: upstream dependency failed", id_str);
    res->state = kResourceFailed;
    orc->run.success = false;
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
  if (orc->run.success) {
    status = kStatusOk;
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(status));
  } else {
    status = kStatusInternalError;
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(status));
  }

  OrchestratorPublish(orc, RECONCILE_FINISHED_EVENT, NewReconcileFinishedEvent(status));
}

static inline void ReconcileWork(uv_work_t* req) {
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Controller* ctrl = task->ctrl;
  Resource* desired = GetResourceInGraph(task->orc->graph, task->index);

  Resource* observed = &task->observed;
  memset(observed, 0, sizeof(Resource));

  if (desired->spec.raw) {
    {
      json_error_t err;
      json_t* doc = json_loads(desired->spec.raw, 0, &err);
      if (!doc) {
        LOG_ERROR("invalid spec doc:\n%s", desired->spec.raw);
        LOG_ERROR("error on line %d: %s", err.line, err.text);
        return;
      }
      desired->spec.doc = doc;
    }

    observed->spec.raw = strdup(desired->spec.raw);
    {
      json_error_t err;
      json_t* doc = json_loads(observed->spec.raw, 0, &err);
      if (!doc) {
        LOG_ERROR("invalid spec doc:\n%s", observed->spec.raw);
        LOG_ERROR("error on line %d: %s", err.line, err.text);
        return;
      }
      observed->spec.doc = doc;
    }
  }
  ASSERT(desired->spec.doc);

  ResourceIdStr desired_id_str;
  ResourceIdCStr(&desired->id, desired_id_str);
  SetLogResourceContext(desired_id_str, desired->kind);
  {
    {
      const ControllerStatus status = ControllerObserve(ctrl, observed, desired);
      if (status != kStatusOk)
        goto finished;
    }

    if (!ControllerValidate(ctrl, desired, &task->vlog))
      goto finished;
    if (IsValidateReconcileTask(task))
      goto finished;

    task->action = ControllerPlan(ctrl, observed, desired, &task->plan);
    if (IsPlanReconcileTask(task))
      goto finished;
    if (task->action == kNoAction)
      goto finished;

    if (IsApplyReconcileTask(task))
      task->status = ControllerApply(ctrl, desired, task->action);
  }
finished:
  ClearLogResourceContext();

  if (desired->spec.doc)
    json_decref(desired->spec.doc);
  desired->spec.doc = NULL;

  if (observed->spec.doc)
    json_decref(observed->spec.doc);
  observed->spec.doc = NULL;
}

static inline void WriteResourceState(Orchestrator* orc, const Resource* res) {
  ASSERT(res->spec.raw);
  ResourceIdStr id_str;
  ResourceIdCStr(&res->id, id_str);

  StateEntry entry = {
      .id = strdup(id_str),
      .kind = strdup(res->kind),
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
  free(entry.id);
  free(entry.kind);
  free(entry.name);
}

static inline void WriteHistory(Orchestrator* orc, const Resource* res, const ControllerAction action,
                                const ControllerStatus status) {
  ResourceIdStr id_str;
  ResourceIdCStr(&res->id, id_str);

  HistoryRecord record = {
      .id = strdup(id_str),
      .kind = strdup(res->kind),
      .action = action,
      .status = status,
      .run_id = 0,
      // char* hash_before;
      // char* hash_after;
      .applied_at = (int64_t)orc->metrics.run_finished,
  };
  memcpy(record.reason, orc->run.reason, sizeof(Reason));  // TODO(@s0cks): should check if reason is not empty first
  LOG_ERROR_IF(!HistoryLogAppend(orc->history, &record), "failed to write to history for: %s", id_str);
}

static inline void ReconcileAfterWork(uv_work_t* req, int status) {
  ASSERT(req);
  ReconcileTask* task = container_of(req, ReconcileTask, work);
  Orchestrator* orc = task->orc;
  Resource* res = GetResourceInGraph(orc->graph, task->index);

  ResourceIdStr res_id_str;
  ResourceIdCStr(&res->id, res_id_str);
  SetLogResourceContext(res_id_str, res->kind);
  orc->metrics.num_actions[task->action]++;

  AppendValidationLog(&orc->vlog, &task->vlog);
  AppendPlan(&orc->plan, &task->plan);
  if (IsApplyReconcileTask(task)) {
    AppliedAction* action = NewAppliedAction(&orc->actions);
    ASSERT(action);
    action->resource = res;
    memcpy(action->reason, task->reason, sizeof(Reason));
    action->action = task->action;
    memcpy(&action->timestamp, &task->start, sizeof(struct timespec));
  }

  if (status != 0) {
    res->state = kResourceFailed;
    orc->run.success = false;
  } else {
    res->state = (task->status == kStatusOk) ? kResourceReady : kResourceFailed;
    orc->run.success = (task->status == kStatusOk);
  }

  WriteResourceState(orc, res);
  WriteHistory(orc, res, task->action, task->status);
  free(task);
  orc->pending--;
  orc->metrics.num_processed++;
  ClearLogResourceContext();

  DispatchReadyResources(orc);
}

static inline bool QueueReconcileTaskForResource(const ResourceGraphIndex idx, Resource* res, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  ResourceGraph* graph = orc->graph;
  if (!IsResourcePending(res))
    return true;

  if (!DependenciesAreSatisfied(orc->graph, res))
    return true;

  Controller* ctrl = GetControllerForKind(res->kind);
  if (!ctrl) {
    ResourceIdStr id_str;
    ResourceIdCStr(&res->id, id_str);
    LOG_ERROR("no controller registered for kind '%s' (resource: %s)", res->kind, id_str);
    res->state = kResourceFailed;
    orc->run.success = false;
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
  ReconcileTask* task = (ReconcileTask*)malloc(sizeof(ReconcileTask));
  memset(task, 0, sizeof(ReconcileTask));
  ResourceIdStr id_str;
  ResourceIdCStr(&res->id, id_str);
  LOG_WARN_IF(!StateStoreGet(orc->state, id_str, &task->last_applied),
              "failed to get last state store value for: %s (%s)", id_str, res->kind);
  task->orc = orc;
  clock_gettime(CLOCK_REALTIME, &task->start);
  task->mode = orc->run.mode;
  task->index = index;
  task->ctrl = ctrl;
  task->action = kNoAction;
  task->status = kStatusOk;
  memset(&task->observed, 0, sizeof(Resource));
  InitPlan(&task->plan, 3);
  InitValidationLog(&task->vlog, 3);

  uv_queue_work(orc->loop, &task->work, ReconcileWork, ReconcileAfterWork);
}
