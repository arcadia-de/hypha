#ifndef HYPHA_HISTORY_LOG_H
#define HYPHA_HISTORY_LOG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/state.h"

struct _HistoryLog {
  StateLog* log;
  char* path;
  uint64_t max_bytes;
  uint32_t keep_rotations;
};

// Bumped whenever the wire layout of an encoded HistoryRecord changes. Mirrors
// STATE_ENTRY_ENCODING_VERSION's discipline: DecodeHistoryRecord rejects anything that
// doesn't match rather than guessing -- a version mismatch means either a genuinely old
// record from before a schema change, or corruption, and either way misinterpreting the
// bytes is worse than dropping the record.
#define HISTORY_RECORD_ENCODING_VERSION 1

// String/array wire format matches libs/state/src/state_store.h's (size_t length prefix,
// 0 meaning absent/empty; no separate presence byte) so the two logs share one mental model
// instead of each log having its own serialization convention.
static inline size_t StringEncodedSize(const char* s) {
  const size_t len = s ? strlen(s) : 0;
  return sizeof(size_t) + len;
}

static inline void PutString(uint8_t** p, const char* s) {
  const size_t len = s ? strlen(s) : 0;
  memcpy(*p, &len, sizeof(size_t));
  *p += sizeof(size_t);
  if (len > 0) {
    memcpy(*p, s, len);
    *p += len;
  }
}

static inline size_t PutArray(uint8_t* dst, const void* src, size_t len, size_t stride) {
  memcpy(dst, &len, sizeof(size_t));
  dst += sizeof(size_t);
  const size_t total_array_size = len * stride;
  if (len > 0)
    memcpy(dst, src, total_array_size);
  return sizeof(size_t) + total_array_size;
}

// Bounds-checked decode helpers -- buf/end delimit the actual bytes read back from the log
// (StateLogRead's value_len), so a truncated, corrupt, or wrong-version record fails cleanly
// (returns false) instead of walking off into whatever memory happens to follow the buffer.
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

static inline bool GetArrayChecked(const uint8_t** p, const uint8_t* end, void** out, size_t* out_len,
                                   size_t stride) {
  if (!HasBytesRemaining(*p, end, sizeof(size_t)))
    return false;

  size_t len = 0;
  memcpy(&len, *p, sizeof(size_t));
  *p += sizeof(size_t);
  if (len == 0) {
    *out = NULL;
    *out_len = 0;
    return true;
  }

  if (!HasBytesRemaining(*p, end, len * stride))
    return false;

  void* buf = malloc(len * stride);
  if (!buf)
    return false;
  memcpy(buf, *p, len * stride);
  *p += len * stride;
  *out = buf;
  *out_len = len;
  return true;
}

static inline uint8_t* EncodeHistoryRecord(const HistoryRecord* rec, uint32_t* out_len) {
  const size_t size = sizeof(uint32_t) +                                            /* version */
                      StringEncodedSize(rec->kind) +                                /* kind */
                      StringEncodedSize(rec->name) +                                /* name */
                      sizeof(uint64_t) +                                            /* hash_before */
                      sizeof(uint64_t) +                                            /* hash_after */
                      sizeof(Reason) +                                              /* reason */
                      sizeof(uint32_t) +                                            /* action */
                      sizeof(uint32_t) +                                            /* status */
                      sizeof(uuid_t) +                                              /* run_id */
                      sizeof(int64_t) +                                             /* applied_at */
                      sizeof(size_t) + (rec->labels_len * sizeof(Label)) +          /* labels */
                      sizeof(size_t) + (rec->annotations_len * sizeof(Annotation)); /* annotations */

  uint8_t* buf = (uint8_t*)malloc(size);
  uint8_t* p = buf;

  const uint32_t version = HISTORY_RECORD_ENCODING_VERSION;
  memcpy(p, &version, sizeof(uint32_t));
  p += sizeof(uint32_t);
  PutString(&p, rec->kind);
  PutString(&p, rec->name);
  memcpy(p, &rec->hash_before, sizeof(uint64_t));
  p += sizeof(uint64_t);
  memcpy(p, &rec->hash_after, sizeof(uint64_t));
  p += sizeof(uint64_t);
  memcpy(p, rec->reason, sizeof(Reason));
  p += sizeof(Reason);
  memcpy(p, &rec->action, sizeof(uint32_t));
  p += sizeof(uint32_t);
  memcpy(p, &rec->status, sizeof(uint32_t));
  p += sizeof(uint32_t);
  memcpy(p, rec->run_id, sizeof(uuid_t));
  p += sizeof(uuid_t);
  memcpy(p, &rec->applied_at, sizeof(int64_t));
  p += sizeof(int64_t);
  p += PutArray(p, rec->labels, rec->labels_len, sizeof(Label));
  p += PutArray(p, rec->annotations, rec->annotations_len, sizeof(Annotation));

  *out_len = (uint32_t)size;
  return buf;
}

// Returns false (and logs) if buf/buf_len don't decode as a well-formed, current-version
// HistoryRecord -- caller should treat `out` as untouched/invalid.
static inline bool DecodeHistoryRecord(const char* id, const uint8_t* buf, size_t buf_len, HistoryRecord* out) {
  const uint8_t* p = buf;
  const uint8_t* end = buf + buf_len;

#define FAIL(msg)                                       \
  do {                                                  \
    LOG_ERROR("failed to decode history record: " msg); \
    HistoryRecordFree(out);                              \
    return false;                                        \
  } while (0)

  memset(out, 0, sizeof(HistoryRecord));
  out->id = strdup(id);

  if (!HasBytesRemaining(p, end, sizeof(uint32_t)))
    FAIL("truncated (missing version)");
  uint32_t version = 0;
  memcpy(&version, p, sizeof(uint32_t));
  p += sizeof(uint32_t);
  if (version != HISTORY_RECORD_ENCODING_VERSION)
    FAIL("unsupported encoding version -- history log likely predates a schema change and needs clearing");

  if (!GetStringChecked(&p, end, &out->kind))
    FAIL("truncated (kind)");
  if (!GetStringChecked(&p, end, &out->name))
    FAIL("truncated (name)");

  if (!HasBytesRemaining(p, end, sizeof(uint64_t)))
    FAIL("truncated (hash_before)");
  memcpy(&out->hash_before, p, sizeof(uint64_t));
  p += sizeof(uint64_t);

  if (!HasBytesRemaining(p, end, sizeof(uint64_t)))
    FAIL("truncated (hash_after)");
  memcpy(&out->hash_after, p, sizeof(uint64_t));
  p += sizeof(uint64_t);

  if (!HasBytesRemaining(p, end, sizeof(Reason)))
    FAIL("truncated (reason)");
  memcpy(out->reason, p, sizeof(Reason));
  p += sizeof(Reason);

  if (!HasBytesRemaining(p, end, sizeof(uint32_t)))
    FAIL("truncated (action)");
  memcpy(&out->action, p, sizeof(uint32_t));
  p += sizeof(uint32_t);

  if (!HasBytesRemaining(p, end, sizeof(uint32_t)))
    FAIL("truncated (status)");
  memcpy(&out->status, p, sizeof(uint32_t));
  p += sizeof(uint32_t);

  if (!HasBytesRemaining(p, end, sizeof(uuid_t)))
    FAIL("truncated (run_id)");
  memcpy(out->run_id, p, sizeof(uuid_t));
  p += sizeof(uuid_t);

  if (!HasBytesRemaining(p, end, sizeof(int64_t)))
    FAIL("truncated (applied_at)");
  memcpy(&out->applied_at, p, sizeof(int64_t));
  p += sizeof(int64_t);

  void* labels = NULL;
  size_t labels_len = 0;
  if (!GetArrayChecked(&p, end, &labels, &labels_len, sizeof(Label)))
    FAIL("truncated (labels)");
  out->labels = (Label*)labels;
  out->labels_len = labels_len;

  void* annotations = NULL;
  size_t annotations_len = 0;
  if (!GetArrayChecked(&p, end, &annotations, &annotations_len, sizeof(Annotation)))
    FAIL("truncated (annotations)");
  out->annotations = (Annotation*)annotations;
  out->annotations_len = annotations_len;

#undef FAIL
  return true;
}

#endif  // HYPHA_HISTORY_LOG_H
