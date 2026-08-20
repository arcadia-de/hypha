#include "state_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/log.h"
#include "hypha/state.h"
#include "state_log.h"

void StateEntryFree(StateEntry* entry) {
  if (!entry)
    return;

  free(entry->id);
  free(entry->kind);
  free(entry->observed_json);
  entry->id = entry->kind = entry->observed_json = NULL;
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
  DecodeStateEntry(value, out);
  free(value);
  success = true;
finished:
  return success;
}

bool StateStorePut(StateStore* store, const StateEntry* entry) {
  uint8_t* value = NULL;
  size_t value_len = 0;
  EncodeStateEntry(entry, &value, &value_len);

  StateLogLocation loc;
  const bool ok = StateLogAppendPut(store->log, entry->id, value, (uint32_t)value_len, &loc);
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
    DecodeStateEntry(value, &entry);
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

void EncodeStateEntry(const StateEntry* entry, uint8_t** buff, size_t* length) {
  const size_t size = StringEncodedSize(entry->kind) +          /* kind */
                      sizeof(uint64_t) +                        /* hash */
                      StringEncodedSize(entry->observed_json) + /* observed_json */
                      sizeof(time_t) +                          /* applied_at */
                      sizeof(uint32_t) +                        /* last_status */
                      1;                                        /* orphaned */
  uint8_t* buf = (uint8_t*)calloc(size, sizeof(uint8_t));
  if (!buf) {
    LOG_ERROR("failed to encode StateEntry");
    return;
  }

  uint8_t* p = &buf[0];
  PutString(&p, entry->kind);

  memcpy(p, &entry->hash, 8);
  p += 8;

  PutString(&p, entry->observed_json);

  memcpy(p, &entry->applied_at, sizeof(time_t));
  p += sizeof(time_t);

  memcpy(p, &entry->last_status, 4);
  p += 4;

  const uint8_t orphaned = entry->orphaned ? 1 : 0;
  memcpy(p, &orphaned, 1);
  p += 1;

  (*buff) = buf;
  (*length) = size;
}

void DecodeStateEntry(const uint8_t* buf, StateEntry* out) {
  const uint8_t* p = buf;
  out->kind = GetString(&p);

  memcpy(&out->hash, p, 8);
  p += 8;

  out->observed_json = GetString(&p);

  memcpy(&out->applied_at, p, sizeof(time_t));
  p += sizeof(time_t);

  memcpy(&out->last_status, p, 4);
  p += 4;

  out->orphaned = *p != 0;
}
