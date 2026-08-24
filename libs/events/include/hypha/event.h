#ifndef HYPHA_EVENT_H
#define HYPHA_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "hypha.h"
#include "hypha/controller_status.h"

#define FOR_EACH_ORCHESTRATOR_EVENT(V) \
  V(OrchestratorInit)                  \
  V(OrchestratorDeInit)

#define FOR_EACH_RECONCILE_EVENT(V) \
  V(ReconcileStarted)               \
  V(ReconcileComplete)              \
  V(ReconcileFailed)                \
  V(ReconcileFinished)

#define FOR_EACH_RESOURCE_EVENT(V) \
  V(ResourceObserved)              \
  V(ResourcePlanned)               \
  V(ResourceNormalized)            \
  V(ResourceValidated)             \
  V(ResourceApplied)               \
  V(ResourceReady)                 \
  V(ResourceRollback)              \
  V(ResourceDestroyed)

#define FOR_EACH_ROLLBACK_EVENT(V) \
  V(RollbackStarted)               \
  V(RollbackFailed)                \
  V(RollbackComplete)              \
  V(RollbackFinished)

#define FOR_EACH_EVENT(V)        \
  V(GraphSubmitted)              \
  FOR_EACH_ORCHESTRATOR_EVENT(V) \
  FOR_EACH_RECONCILE_EVENT(V)    \
  FOR_EACH_RESOURCE_EVENT(V)     \
  FOR_EACH_ROLLBACK_EVENT(V)

// clang-format off
typedef enum {
  kInvalidEvent = 0,
#define DEFINE_KIND(Name) k##Name##Event,
  FOR_EACH_EVENT(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfEventKinds,
} EventKind;
// clang-format on

#define GRAPH_SUBMITTED_EVENT     "graph.submitted"

#define ORCHESTRATOR_INIT_EVENT   "orchestrator.init"
#define ORCHESTRATOR_DEINIT_EVENT "orchestrator.deinit"

#define RECONCILE_STARTED_EVENT   "reconcile.started"
#define RECONCILE_COMPLETE_EVENT  "reconcile.complete"
#define RECONCILE_FAILED_EVENT    "reconcile.failed"
#define RECONCILE_FINISHED_EVENT  "reconcile.finished"

#define RESOURCE_OBSERVED_EVENT   "resource.observed"
#define RESOURCE_PLANNED_EVENT    "resource.planned"
#define RESOURCE_NORMALIZED_EVENT "resource.normalized"
#define RESOURCE_VALIDATED_EVENT  "resource.validated"
#define RESOURCE_APPLIED_EVENT    "resource.applied"
#define RESOURCE_READY_EVENT      "resource.ready"
#define RESOURCE_ROLLBACK_EVENT   "resource.rollback"
#define RESOURCE_DESTROYED_EVENT  "resource.destroyed"

#define ROLLBACK_STARTED_EVENT    "rollback.started"
#define ROLLBACK_FAILED_EVENT     "rollback.failed"
#define ROLLBACK_COMPLETE_EVENT   "rollback.complete"
#define ROLLBACK_FINISHED_EVENT   "rollback.finished"

#define DEFINE_EVENT_FIELDS \
  EventKind kind;           \
  uint64_t timestamp;

typedef struct {
  DEFINE_EVENT_FIELDS;
} GraphSubmittedEvent;

GraphSubmittedEvent* NewGraphSubmittedEvent();
void FreeGraphSubmittedEvent(GraphSubmittedEvent*);

// ╭─────────────────────╮
// │ Orchestrator Events │
// ╰─────────────────────╯
#define DEFINE_ORCHESTRATOR_EVENT_FIELDS DEFINE_EVENT_FIELDS;

#define DECLARE_ORCHESTRATOR_EVENT(Name)                     \
  typedef struct {                                           \
    DEFINE_ORCHESTRATOR_EVENT_FIELDS;                        \
  } Orchestrator##Name##Event;                               \
  Orchestrator##Name##Event* NewOrchestrator##Name##Event(); \
  void FreeOrchestrator##Name##Event(Orchestrator##Name##Event*);

DECLARE_ORCHESTRATOR_EVENT(Init);
DECLARE_ORCHESTRATOR_EVENT(DeInit);
#undef DECLARE_ORCHESTRATOR_EVENT
#undef DEFINE_ORCHESTRATOR_EVENT_FIELDS
// ──────────────────────────────────────────────────────────────────────

// ╭──────────────────╮
// │ Reconcile Events │
// ╰──────────────────╯
typedef struct {
  DEFINE_EVENT_FIELDS;
} ReconcileStartedEvent;

ReconcileStartedEvent* NewReconcileStartedEvent();
void FreeReconcileStartedEvent(ReconcileStartedEvent*);

#define DEFINE_RECONCILE_EVENT_FIELDS \
  DEFINE_EVENT_FIELDS;                \
  ControllerStatus status;

#define DECLARE_RECONCILE_EVENT(Name)                                               \
  typedef struct {                                                                  \
    DEFINE_RECONCILE_EVENT_FIELDS;                                                  \
  } Reconcile##Name##Event;                                                         \
  Reconcile##Name##Event* NewReconcile##Name##Event(const ControllerStatus status); \
  void FreeReconcile##Name##Event(Reconcile##Name##Event*);

DECLARE_RECONCILE_EVENT(Complete);
DECLARE_RECONCILE_EVENT(Failed);
DECLARE_RECONCILE_EVENT(Finished);
#undef DECLARE_RECONCILE_EVENT
#undef DEFINE_RECONCILE_EVENT_FIELDS
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────╮
// │ Resource Events │
// ╰─────────────────╯
#define DEFINE_RESOURCE_EVENT_FIELDS \
  DEFINE_EVENT_FIELDS;               \
  const Resource* resource;

#define DECLARE_RESOURCE_EVENT(Name)                                \
  typedef struct {                                                  \
    DEFINE_RESOURCE_EVENT_FIELDS;                                   \
  } Resource##Name##Event;                                          \
  Resource##Name##Event* NewResource##Name##Event(const Resource*); \
  void FreeResource##Name##Event(Resource##Name##Event*);

DECLARE_RESOURCE_EVENT(Observed);
DECLARE_RESOURCE_EVENT(Planned);
DECLARE_RESOURCE_EVENT(Normalized);
DECLARE_RESOURCE_EVENT(Validated);
DECLARE_RESOURCE_EVENT(Applied);
DECLARE_RESOURCE_EVENT(Ready);
DECLARE_RESOURCE_EVENT(Rollback);
DECLARE_RESOURCE_EVENT(Destroyed);
#undef DECLARE_RESOURCE_EVENT
#undef DEFINE_RESOURCE_EVENT_FIELDS
// ──────────────────────────────────────────────────────────────────────

// ╭─────────────────╮
// │ Rollback Events │
// ╰─────────────────╯
#define DEFINE_ROLLBACK_EVENT_FIELDS DEFINE_EVENT_FIELDS;

#define DECLARE_ROLLBACK_EVENT(Name)                 \
  typedef struct {                                   \
    DEFINE_ROLLBACK_EVENT_FIELDS;                    \
  } Rollback##Name##Event;                           \
  Rollback##Name##Event* NewRollback##Name##Event(); \
  void FreeRollback##Name##Event(Rollback##Name##Event*);

DECLARE_ROLLBACK_EVENT(Started);
DECLARE_ROLLBACK_EVENT(Failed);
DECLARE_ROLLBACK_EVENT(Complete);
DECLARE_ROLLBACK_EVENT(Finished);
#undef DEFINE_ROLLBACK_EVENT_FIELDS
#undef DECLARE_ROLLBACK_EVENT
// ──────────────────────────────────────────────────────────────────────

#define HYPHA_EVENT_ALPHABET_SIZE 31

typedef bool (*EventCallbackFn)(const char* p, const void* event, void* data);

typedef struct _EventRoute EventRoute;

EventRoute* NewEventRoute(void);
bool Subscribe(EventRoute* root, const char* p, EventCallbackFn cb, void* data, void (*free_data)(void*));
bool Publish(EventRoute* root, const char* p, void* event);
void FreeEventRoute(EventRoute* root);

#ifndef HYPHA_EVENT_BUS_INIT_CAP
#define HYPHA_EVENT_BUS_INIT_CAP 16
#endif  // HYPHA_EVENT_BUS_INIT_CAP

typedef struct {
  const char* event;
  uintptr_t data;
} ScheduledEvent;

typedef struct {
  uv_async_t async;
  uv_mutex_t mutex;

  EventRoute* root;

  ScheduledEvent* queue;
  size_t queue_len;
  size_t queue_cap;
} EventBus;

void InitEventBus(uv_loop_t* loop, EventBus* bus);
void EventBusPublish(EventBus* bus, const char* p, void* data);
void EventBusSubscribe(EventBus* bus, const char* p, EventCallbackFn cb, void* data, void (*free_data)(void*));
void FreeEventBus(EventBus* bus);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_EVENT_H
