#ifndef HYPHA_RECONCILE_H
#define HYPHA_RECONCILE_H

#include <uv.h>

#include "hypha.h"
#include "hypha/orchestrator.h"
#include "hypha/state.h"

typedef struct {
  uv_work_t work;
  Orchestrator* orc;
  ResourceGraphIndex index;
  Controller* ctrl;
  Resource observed;
  ControllerAction action;
  ControllerStatus status;
  StateEntry last_applied;
} ReconcileTask;

#endif  // HYPHA_RECONCILE_H
