#ifndef HYPHA_EVENT_BUS_H
#define HYPHA_EVENT_BUS_H

#include <uv.h>

#include "hypha/event.h"

typedef struct _Subscription {
  struct _Subscription* next;

  OrchestratorEventHandlerFn fn;
  void* data;
} Subscription;

struct _EventBus {
  uv_async_t handle;
  uv_mutex_t mutex;
  OrchestratorEvent* queue;
  uint32_t count;
  uint32_t capacity;
  Subscription subscriptions[kTotalNumberOfOrchestratorEvents];
};

void InitEventBus(uv_loop_t* loop, EventBus* bus);
void EventBusEmit(EventBus* bus, const OrchestratorEvent* event);
void FreeEventBus(EventBus* bus);

#endif  // HYPHA_EVENT_BUS_H
