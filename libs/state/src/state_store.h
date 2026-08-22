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

// Bounds-checked decode helpers. buf/end delimit the actual bytes read back from
// the log (StateLogRead's value_len), so a truncated, corrupt, or wrong-version
// entry fails cleanly (returns false, logs) instead of walking off into whatever
// memory happens to follow the buffer.
static inline bool HasBytesRemaining(const uint8_t* p, const uint8_t* end, size_t need) {
  return p <= end && need <= (size_t)(end - p);
}

static inline bool GetStringChecked(const uint8_t** p, const uint8_t* end, char** out) {
  if (!HasBytesRemaining(*p, end, sizeof(size_t)))
    return false;

  size_t len = 0;
  memcpy(&len, *p, sizeof(size_t));
  *p += sizeof(size_t);
  if (len == 0) {
    *out = NULL;
    return true;
  }

  if (!HasBytesRemaining(*p, end, len))
    return false;

  char* s = (char*)malloc(len + 1);
  if (!s)
    return false;
  memcpy(s, *p, len);
  s[len] = '\0';
  *p += len;
  *out = s;
  return true;
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
