#ifndef HYPHA_STATE_STORE_H
#define HYPHA_STATE_STORE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/state.h"
#include "state_index.h"
#include "state_log.h"

struct _StateStore {
  StateLog* log;
  Index index;
};

static inline void PutString(uint8_t** p, const char* s) {
  const size_t len = s ? strlen(s) : 0;
  memcpy(*p, &len, sizeof(size_t));
  (*p) += sizeof(size_t);
  if (len > 0) {
    memcpy(*p, s, len);
    (*p) += len;
  }
}

static inline size_t StringEncodedSize(const char* s) {
  size_t size = sizeof(size_t);
  size_t len = s ? strlen(s) : 0;
  if (len > 0)
    size += len;
  return size;
}

static inline char* GetString(const uint8_t** p) {
  size_t len = 0;
  memcpy(&len, *p, sizeof(size_t));
  *p += sizeof(size_t);
  if (len == 0)
    return NULL;

  char* s = (char*)malloc(len + 1);
  memcpy(s, *p, len);
  s[len] = '\0';
  *p += len;
  return s;
}

static inline bool ReplayCallback(const char* key, bool tombstone, StateLogLocation loc, void* data) {
  Index* idx = (Index*)data;
  if (tombstone) {
    IndexRemove(idx, key);
  } else {
    IndexSet(idx, key, loc);
  }

  return true;
}

#endif  // HYPHA_STATE_STORE_H
