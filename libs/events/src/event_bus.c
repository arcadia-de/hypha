#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "hypha/event.h"
#include "hypha/log.h"

static inline bool EventBusPop(EventBus* bus, ScheduledEvent* next) {
  bool result = false;
  uv_mutex_lock(&bus->mutex);
  if (bus->queue_len > 0) {
    memmove(next, &bus->queue[0], sizeof(ScheduledEvent));
    memmove(&bus->queue[0], &bus->queue[1], sizeof(ScheduledEvent) * (bus->queue_len - 1));
    bus->queue_len--;
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

  ScheduledEvent next;
  while (EventBusPop(bus, &next)) {
    Publish(bus->root, next.event, (void*)next.data);

    // TODO(@s0cks): causes double free
    //  free((void*)next.event);
  }
}

void InitEventBus(uv_loop_t* loop, EventBus* bus) {
  if (!loop || !bus)
    return;

  memset(bus, 0, sizeof(EventBus));
  uv_mutex_init(&bus->mutex);
  uv_async_init(loop, &bus->async, &OnEventBusTriggered);
  bus->async.data = bus;

  const size_t init_cap = HYPHA_EVENT_BUS_INIT_CAP;
  const size_t total_queue_size = sizeof(ScheduledEvent) * init_cap;
  ScheduledEvent* queue = (ScheduledEvent*)malloc(total_queue_size);
  if (queue) {
    memset(queue, 0, total_queue_size);
    bus->root = NewEventRoute();
    bus->queue = queue;
    bus->queue_cap = init_cap;
    bus->queue_len = 0;
  }
}

void EventBusSubscribe(EventBus* bus, const char* p, EventCallbackFn cb, void* data, void (*free_data)(void*)) {
  if (!bus)
    return;

  Subscribe(bus->root, p, cb, data, free_data);
}

void EventBusPublish(EventBus* bus, const char* p, void* next) {
  if (!bus)
    return;

  uv_mutex_lock(&bus->mutex);
  if (bus->queue_len == bus->queue_cap) {
    const uint32_t new_cap = bus->queue_cap * 2;
    ScheduledEvent* new_queue = (ScheduledEvent*)realloc(bus->queue, sizeof(ScheduledEvent) * new_cap);
    if (!new_queue)
      goto finished;

    bus->queue = new_queue;
    bus->queue_cap = new_cap;
  }

  ScheduledEvent* next_event = (ScheduledEvent*)&bus->queue[bus->queue_len];
  bus->queue_len++;

  memset(next_event, 0, sizeof(ScheduledEvent));
  next_event->event = strdup(p);
  next_event->data = (uintptr_t)next;

  uv_async_send(&bus->async);
finished:
  uv_mutex_unlock(&bus->mutex);
  return;
}

void FreeEventBus(EventBus* bus) {
  if (!bus)
    return;

  uv_mutex_destroy(&bus->mutex);
  uv_close((uv_handle_t*)&bus->async, NULL);

  if (bus->root)
    FreeEventRoute(bus->root);

  if (bus->queue)
    free(bus->queue);

  free(bus);
}
