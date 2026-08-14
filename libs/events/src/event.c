#include "hypha/event.h"

#include <stdlib.h>

#include "event_trie.h"
#include "hypha/log.h"

GraphSubmittedEvent* NewGraphSubmittedEvent() {
  GraphSubmittedEvent* event = (GraphSubmittedEvent*)malloc(sizeof(GraphSubmittedEvent));
  if (event) {
    event->kind = kGraphSubmittedEvent;
    event->timestamp = 0;
  }
  return event;
}

void FreeGraphSubmittedEvent(GraphSubmittedEvent* event) {
  if (!event)
    return;

  free(event);
}

// ╭──────────────────╮
// │ Reconcile Events │
// ╰──────────────────╯
ReconcileStartedEvent* NewReconcileStartedEvent() {
  ReconcileStartedEvent* event = (ReconcileStartedEvent*)malloc(sizeof(ReconcileStartedEvent));
  if (event) {
    event->kind = kReconcileStartedEvent;
    event->timestamp = 0;
  }
  return event;
}

void FreeReconcileStartedEvent(ReconcileStartedEvent* event) {
  if (!event)
    return;

  free(event);
}

#define DEFINE_RECONCILE_EVENT(Name)                                                                 \
  Reconcile##Name##Event* NewReconcile##Name##Event(const ControllerStatus status) {                 \
    Reconcile##Name##Event* event = (Reconcile##Name##Event*)malloc(sizeof(Reconcile##Name##Event)); \
    if (event) {                                                                                     \
      event->kind = kReconcile##Name##Event;                                                         \
      event->timestamp = 0;                                                                          \
      event->status = status;                                                                        \
    }                                                                                                \
    return event;                                                                                    \
  }                                                                                                  \
  void FreeReconcile##Name##Event(Reconcile##Name##Event* event) {                                   \
    if (!event)                                                                                      \
      return;                                                                                        \
    free(event);                                                                                     \
  }

DEFINE_RECONCILE_EVENT(Failed);
DEFINE_RECONCILE_EVENT(Complete);
DEFINE_RECONCILE_EVENT(Finished);
#undef DEFINE_RECONCILE_EVENT
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────╮
// │ Resource Events │
// ╰─────────────────╯
#define DEFINE_RESOURCE_EVENT(Name)                                                               \
  Resource##Name##Event* NewResource##Name##Event(const Resource* res) {                          \
    Resource##Name##Event* event = (Resource##Name##Event*)malloc(sizeof(Resource##Name##Event)); \
    if (event) {                                                                                  \
      event->kind = kResource##Name##Event;                                                       \
      event->timestamp = 0;                                                                       \
      event->resource = res;                                                                      \
    }                                                                                             \
    return event;                                                                                 \
  }                                                                                               \
  void FreeResource##Name##Event(Resource##Name##Event* event) {                                  \
    if (!event)                                                                                   \
      return;                                                                                     \
    free(event);                                                                                  \
  }

DEFINE_RESOURCE_EVENT(Observed);
DEFINE_RESOURCE_EVENT(Planned);
DEFINE_RESOURCE_EVENT(Normalized);
DEFINE_RESOURCE_EVENT(Validated);
DEFINE_RESOURCE_EVENT(Applied);
DEFINE_RESOURCE_EVENT(Ready);
DEFINE_RESOURCE_EVENT(Rollback);
DEFINE_RESOURCE_EVENT(Destroyed);
#undef DEFINE_RESOURCE_EVENT
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────────╮
// │ Orchestrator Events │
// ╰─────────────────────╯
#define DEFINE_ORCHESTRATOR_EVENT(Name)                                                                       \
  Orchestrator##Name##Event* NewOrchestrator##Name##Event() {                                                 \
    Orchestrator##Name##Event* event = (Orchestrator##Name##Event*)malloc(sizeof(Orchestrator##Name##Event)); \
    if (event) {                                                                                              \
      event->kind = kOrchestrator##Name##Event;                                                               \
      event->timestamp = 0;                                                                                   \
    }                                                                                                         \
    return event;                                                                                             \
  }                                                                                                           \
  void FreeOrchestrator##Name##Event(Orchestrator##Name##Event* event) {                                      \
    if (!event)                                                                                               \
      return;                                                                                                 \
    free(event);                                                                                              \
  }

DEFINE_ORCHESTRATOR_EVENT(Init);
DEFINE_ORCHESTRATOR_EVENT(DeInit);
#undef DEFINE_ORCHESTRATOR_EVENT
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────╮
// │ Rollback Events │
// ╰─────────────────╯
#define DEFINE_ROLLBACK_EVENT(Name)                                                               \
  Rollback##Name##Event* NewRollback##Name##Event() {                                             \
    Rollback##Name##Event* event = (Rollback##Name##Event*)malloc(sizeof(Rollback##Name##Event)); \
    if (event) {                                                                                  \
      event->kind = kRollback##Name##Event;                                                       \
      event->timestamp = 0;                                                                       \
    }                                                                                             \
    return event;                                                                                 \
  }                                                                                               \
  void FreeRollback##Name##Event(Rollback##Name##Event* event) {                                  \
    if (!event)                                                                                   \
      return;                                                                                     \
    free(event);                                                                                  \
  }

DEFINE_ROLLBACK_EVENT(Started);
DEFINE_ROLLBACK_EVENT(Failed);
DEFINE_ROLLBACK_EVENT(Complete);
DEFINE_ROLLBACK_EVENT(Finished);
// ──────────────────────────────────────────────────────────────────────
