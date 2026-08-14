#include "state_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/log.h"
#include "state_log.h"

void StateEntryFree(StateEntry* entry) {
  if (!entry)
    return;

  free(entry->id);
  free(entry->kind);
  free(entry->hash);
  free(entry->observed_json);
  entry->id = entry->kind = entry->hash = entry->observed_json = NULL;
}

StateStore* StateStoreOpen(const char* path) {
  StateLog* log = StateLogOpen(path);
  if (!log)
    return NULL;

  StateStore* store = (StateStore*)malloc(sizeof(StateStore));
  store->log = log;
  IndexInit(&store->index, 64);

  StateLogReplay(log, ReplayCallback, &store->index);
  return store;
}

void StateStoreClose(StateStore* store) {
  if (!store)
    return;

  IndexFree(&store->index);
  StateLogClose(store->log);
  free(store);
}

bool StateStoreGet(StateStore* store, const char* id, StateEntry* out) {
  bool success = false;
  StateLogLocation loc;
  if (!IndexGet(&store->index, id, &loc))
    goto finished;

  uint8_t* value = NULL;
  uint32_t value_len = 0;
  if (!StateLogRead(store->log, loc, &value, &value_len))
    goto finished;

  memset(out, 0, sizeof(StateEntry));
  out->id = strdup(id);
  DecodeEntry(value, out);
  free(value);
  success = true;
finished:
  return success;
}

bool StateStorePut(StateStore* store, const StateEntry* entry) {
  uint32_t value_len = 0;
  uint8_t* value = EncodeEntry(entry, &value_len);

  StateLogLocation loc;
  const bool ok = StateLogAppendPut(store->log, entry->id, value, value_len, &loc);
  free(value);

  if (ok)
    IndexSet(&store->index, entry->id, loc);
  return ok;
}

bool StateStoreDelete(StateStore* store, const char* id) {
  bool success = false;
  if (!StateLogAppendTombstone(store->log, id))
    goto finished;

  IndexRemove(&store->index, id);
  success = true;
finished:
  return success;
}

bool StateStoreVisitAll(StateStore* store, StateStoreVisitFn visit, void* data) {
  for (uint32_t i = 0; i < store->index.capacity; i++) {
    IndexSlot* slot = &store->index.slots[i];
    if (IsIndexSlotKeyValid(slot))
      continue;

    uint8_t* value = NULL;
    uint32_t value_len = 0;
    if (!StateLogRead(store->log, slot->loc, &value, &value_len))
      continue;

    StateEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.id = strdup(slot->key);
    DecodeEntry(value, &entry);
    free(value);

    const bool keep_going = visit(&entry, data);
    StateEntryFree(&entry);

    if (!keep_going)
      goto finished;
  }

finished:
  return true;
}

uint32_t StateStoreCount(StateStore* store) {
  return store->index.count;
}

bool StateStoreFlush(StateStore* store) {
  return StateLogFlush(store->log);
}
