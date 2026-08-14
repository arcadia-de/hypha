#include "hypha/event.h"

#include <stdlib.h>
#include <uv.h>

#include "event_bus.h"
#include "hypha.h"
#include "hypha/log.h"
#include "hypha/orchestrator.h"

static inline bool EventBusPop(EventBus* bus, OrchestratorEvent* out) {
  ASSERT(bus);
  bool result = false;
  uv_mutex_lock(&bus->mutex);
  if (bus->count > 0) {
    (*out) = bus->queue[0];
    memmove(&bus->queue[0], &bus->queue[1], sizeof(OrchestratorEvent) * (bus->count - 1));
    bus->count--;
    result = true;
  }

  uv_mutex_unlock(&bus->mutex);
  return result;
}

static inline void OnEventBusTriggered(uv_async_t* handle) {
  if (!handle)
    return;

  EventBus* bus = (EventBus*)handle->data;
  if (!bus)
    return;

  Orchestrator* orc = EventBusGetOrchestrator(bus);
  OrchestratorEvent event;
  while (EventBusPop(bus, &event)) {
    Subscription* sub = &bus->subscriptions[event.kind];
    if (!sub->fn)
      continue;

    while (sub != NULL) {
      sub->fn(orc, &event, sub->data);
      sub = sub->next;
    }
  }
}

void InitEventBus(uv_loop_t* loop, EventBus* bus) {
  if (!loop || !bus)
    return;

  memset(bus, 0, sizeof(EventBus));
  bus->capacity = 16;
  bus->queue = (OrchestratorEvent*)malloc(sizeof(OrchestratorEvent) * bus->capacity);
  uv_mutex_init(&bus->mutex);
  uv_async_init(loop, &bus->handle, &OnEventBusTriggered);
  bus->handle.data = bus;

  for (int i = 0; i < kTotalNumberOfOrchestratorEvents; i++)
    memset(&bus->subscriptions[i], 0, sizeof(Subscription));
}

void OrchestratorOnEvent(OrchestratorHandle handle, const OrchestratorEventKind kind, OrchestratorEventHandlerFn fn,
                         void* data) {
  ASSERT(handle);
  EventBus* bus = OrchestratorGetEventBus(handle);
  Subscription* head = &bus->subscriptions[kind];
  if (!head->fn) {
    head->fn = fn;
    head->data = data;
    head->next = NULL;  // NOLINT(modernize-use-nullptr)
    return;
  }

  Subscription* last = head;
  while (last->next != NULL)
    last = last->next;

  Subscription* node = (Subscription*)malloc(sizeof(Subscription));
  node->fn = fn;
  node->data = data;
  node->next = NULL;  // NOLINT(modernize-use-nullptr)
  last->next = node;
}

void EventBusEmit(EventBus* bus, const OrchestratorEvent* event) {
  if (!bus || !event)
    return;

  uv_mutex_lock(&bus->mutex);
  if (bus->count == bus->capacity) {
    const uint32_t new_cap = bus->capacity * 2;
    OrchestratorEvent* new_queue = (OrchestratorEvent*)realloc(bus->queue, sizeof(OrchestratorEvent*) * new_cap);
    if (!new_queue) {
      uv_mutex_unlock(&bus->mutex);
      LOG_ERROR("failed to grow EventBus queue for %u entries, dropping event", bus->capacity);
      return;
    }

    bus->queue = new_queue;
    bus->capacity = new_cap;
  }

  bus->queue[bus->count++] = *event;
  uv_mutex_unlock(&bus->mutex);
  uv_async_send(&bus->handle);
}

void OrchestratorPublish(OrchestratorHandle handle, const OrchestratorEvent* event) {
  if (!handle || !event)
    return;
  EventBus* bus = OrchestratorGetEventBus(handle);
  EventBusEmit(bus, event);
}

void FreeEventBus(EventBus* bus) {
  if (!bus)
    return;

  for (int i = 0; i < kTotalNumberOfOrchestratorEvents; i++) {
    Subscription* node = bus->subscriptions[i].next;
    while (node != NULL) {
      Subscription* next = node->next;
      free(node);
      node = next;
    }
  }

  free(bus->queue);
  uv_mutex_destroy(&bus->mutex);
}
