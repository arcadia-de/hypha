#include "state_index.h"

#include <stdlib.h>
#include <string.h>

static char DELETED_MARKER_STORAGE;
#define DELETED_MARKER (&DELETED_MARKER_STORAGE)

static inline uint32_t Fnv1aHash(const char* s) {
  uint32_t h = 2166136261u;
  for (; *s; s++) {
    h ^= (uint8_t)*s;
    h *= 16777619u;
  }

  return h;
}

void IndexInit(Index* idx, uint32_t capacity) {
  idx->capacity = capacity;
  idx->count = 0;
  idx->slots = (IndexSlot*)calloc(capacity, sizeof(IndexSlot));
}

void IndexFree(Index* idx) {
  for (uint32_t i = 0; i < idx->capacity; i++) {
    if (idx->slots[i].key && idx->slots[i].key != DELETED_MARKER)
      free(idx->slots[i].key);
  }

  free(idx->slots);
}

void IndexSet(Index* idx, const char* key, StateLogLocation loc) {
  if ((idx->count + 1) * 10 >= idx->capacity * 7)
    IndexGrow(idx);

  uint32_t i = Fnv1aHash(key) % idx->capacity;
  int32_t first_deleted = -1;

  for (;;) {
    IndexSlot* slot = &idx->slots[i];
    if (slot->key == NULL) {
      const uint32_t target = (first_deleted >= 0) ? (uint32_t)first_deleted : i;
      idx->slots[target].key = strdup(key);
      idx->slots[target].loc = loc;
      idx->count++;
      return;
    }

    if (slot->key == DELETED_MARKER) {
      if (first_deleted < 0)
        first_deleted = (int32_t)i;
    } else if (strcmp(slot->key, key) == 0) {
      slot->loc = loc;
      return;
    }

    i = (i + 1) % idx->capacity;
  }
}

bool IndexGet(Index* idx, const char* key, StateLogLocation* out_loc) {
  if (idx->capacity == 0)
    return false;

  uint32_t i = Fnv1aHash(key) % idx->capacity;
  for (uint32_t probes = 0; probes < idx->capacity; probes++) {
    IndexSlot* slot = &idx->slots[i];
    if (slot->key == NULL)
      return false;

    if (slot->key != DELETED_MARKER && strcmp(slot->key, key) == 0) {
      *out_loc = slot->loc;
      return true;
    }

    i = (i + 1) % idx->capacity;
  }

  return false;
}

void IndexRemove(Index* idx, const char* key) {
  if (idx->capacity == 0)
    return;

  uint32_t i = Fnv1aHash(key) % idx->capacity;
  for (uint32_t probes = 0; probes < idx->capacity; probes++) {
    IndexSlot* slot = &idx->slots[i];
    if (slot->key == NULL)
      return;

    if (slot->key != DELETED_MARKER && strcmp(slot->key, key) == 0) {
      free(slot->key);
      slot->key = DELETED_MARKER;
      idx->count--;
      return;
    }

    i = (i + 1) % idx->capacity;
  }
}

void IndexGrow(Index* idx) {
  Index bigger;
  IndexInit(&bigger, idx->capacity * 2);

  for (uint32_t i = 0; i < idx->capacity; i++) {
    IndexSlot* slot = &idx->slots[i];
    if (slot->key && slot->key != DELETED_MARKER)
      IndexSet(&bigger, slot->key, slot->loc);
  }

  IndexFree(idx);
  *idx = bigger;
}

bool IsIndexSlotKeyValid(IndexSlot* rhs) {
  return !rhs->key || rhs->key == DELETED_MARKER;
}
