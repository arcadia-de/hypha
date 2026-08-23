#ifndef HYPHA_RECONCILE_H
#define HYPHA_RECONCILE_H

#include <uv.h>

#include "hypha.h"
#include "hypha/controller.h"
#include "hypha/orchestrator.h"
#include "hypha/state.h"

typedef struct {
  uv_work_t work;

  Orchestrator* orc;
  OrchestratorRunMode mode;
  ResourceGraphIndex index;
  Controller* ctrl;
  Resource observed;
  ControllerAction action;
  ControllerStatus status;
  StateEntry last_applied;
  bool has_last_applied;  // whether last_applied was actually populated from the state store
  struct timespec start;
  Reason reason;
  Plan plan;
  ValidationLog vlog;
} ReconcileTask;

#define DEFINE_RUN_MODE_CHECK(Name)                                \
  static inline bool Is##Name##ReconcileTask(ReconcileTask* rhs) { \
    return rhs && rhs->mode == kOrchestrator##Name##Mode;          \
  }

FOR_EACH_ORCHESTRATOR_RUN_MODE(DEFINE_RUN_MODE_CHECK)
#undef DEFINE_RUN_MODE_CHECK

#endif  // HYPHA_RECONCILE_H
