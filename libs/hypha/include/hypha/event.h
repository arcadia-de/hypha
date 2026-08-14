#ifndef HYPHA_EVENT_H
#define HYPHA_EVENT_H

#include "hypha.h"
#include "hypha/controller.h"

typedef struct _EventBus EventBus;

#define FOR_EACH_ORCHESTRATOR_EVENT(V) \
  V(GraphSubmitted)                    \
  V(ResourceDecorated)                 \
  V(ReconcileComplete)                 \
  V(ReconcileFailed)

// clang-format off
typedef enum {
  kNoEvent = 0,
#define DEFINE_EVENT(Name) k##Name##Event,
  FOR_EACH_ORCHESTRATOR_EVENT(DEFINE_EVENT)
#undef DEFINE_EVENT
  kTotalNumberOfOrchestratorEvents,
} OrchestratorEventKind;
// clang-format on

typedef struct {
  OrchestratorEventKind kind;
  ControllerAction action;
  ControllerStatus status;
  Resource* resource;
} OrchestratorEvent;

#define DEFINE_TYPE_CHECK(Name)                                        \
  static inline bool Is##Name##Event(const OrchestratorEvent* event) { \
    return event && event->kind == k##Name##Event;                     \
  }
FOR_EACH_ORCHESTRATOR_EVENT(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK

OrchestratorHandle EventBusGetOrchestrator(EventBus*);

typedef void (*OrchestratorEventHandlerFn)(OrchestratorHandle, const OrchestratorEvent*, void*);

#endif  // HYPHA_EVENT_H
