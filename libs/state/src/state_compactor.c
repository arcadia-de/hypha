#include <stdlib.h>
#include <string.h>

#include "state_store.h"

typedef struct {
  StateLogKeepEntry* entries;
  uint8_t** owned_values;
  uint32_t count;
  uint32_t capacity;
} CompactCollector;

static inline bool CollectForCompact(const StateEntry* entry, void* data) {
  CompactCollector* c = (CompactCollector*)data;
  if (c->count == c->capacity)
    return true;

  uint32_t value_len = 0;
  uint8_t* value = EncodeEntry(entry, &value_len);

  c->entries[c->count].key = strdup(entry->id);
  c->entries[c->count].value = value;
  c->entries[c->count].value_len = value_len;
  c->owned_values[c->count] = value;
  c->count++;
  return true;
}

bool StateStoreCompact(StateStore* store) {
  CompactCollector c;
  c.capacity = store->index.count;
  c.entries = (StateLogKeepEntry*)malloc(sizeof(StateLogKeepEntry) * (c.capacity ? c.capacity : 1));
  c.owned_values = (uint8_t**)malloc(sizeof(uint8_t*) * (c.capacity ? c.capacity : 1));
  c.count = 0;

  StateStoreVisitAll(store, CollectForCompact, &c);

  const bool ok = StateLogCompact(store->log, c.entries, c.count);

  if (ok) {
    IndexFree(&store->index);
    IndexInit(&store->index, 64);
    StateLogReplay(store->log, ReplayCallback, &store->index);
  }

  for (uint32_t i = 0; i < c.count; i++) {
    free((void*)c.entries[i].key);
    free(c.owned_values[i]);
  }

  free(c.entries);
  free(c.owned_values);
  return ok;
}
