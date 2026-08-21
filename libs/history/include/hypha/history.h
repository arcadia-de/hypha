#ifndef HYPHA_HISTORY_H
#define HYPHA_HISTORY_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/controller.h"
#include "hypha/state.h"

typedef struct {
  char* id;
  char* kind;
  ControllerAction action;
  ControllerStatus status;
  char* hash_before;
  char* hash_after;
  Reason reason;
  uint64_t run_id;
  int64_t applied_at;
} HistoryRecord;

void HistoryRecordFree(HistoryRecord* rec);

HistoryLog* HistoryLogOpen(const char* path, uint64_t max_bytes, uint32_t keep_rotations);
void HistoryLogClose(HistoryLog* log);
bool HistoryLogAppend(HistoryLog* log, const HistoryRecord* rec);
bool HistoryLogFlush(HistoryLog* log);

typedef bool (*HistoryLogVisitFn)(const HistoryRecord* rec, void* data);
void HistoryLogReplay(HistoryLog* log, HistoryLogVisitFn fn, void* data);

struct _HistoryLog {
  StateLog* log;
  char* path;
  uint64_t max_bytes;
  uint32_t keep_rotations;
};

static inline void PutString(uint8_t** p, const char* s) {
  if (!s) {
    uint8_t present = 0;
    memcpy(*p, &present, 1);
    *p += 1;
    return;
  }

  uint8_t present = 1;
  memcpy(*p, &present, 1);
  *p += 1;
  const uint32_t len = (uint32_t)strlen(s);
  memcpy(*p, &len, 4);
  *p += 4;
  memcpy(*p, s, len);
  *p += len;
}

static inline uint32_t StringEncodedSize(const char* s) {
  return s ? (1 + 4 + (uint32_t)strlen(s)) : 1;
}

static inline char* GetString(const uint8_t** p) {
  const uint8_t present = **p;
  *p += 1;
  if (!present)
    return NULL;

  uint32_t len = 0;
  memcpy(&len, *p, 4);
  *p += 4;
  char* s = (char*)malloc(len + 1);
  memcpy(s, *p, len);
  s[len] = '\0';
  *p += len;
  return s;
}

static inline uint8_t* EncodeEntry(const StateEntry* entry, uint32_t* out_len) {
  const uint32_t size = StringEncodedSize(entry->kind) + 8 + StringEncodedSize(entry->observed_json) +
                        8 /*applied_at*/ + 4 /*last_status*/ + 1 /*orphaned*/;

  uint8_t* buf = (uint8_t*)malloc(size);
  uint8_t* p = buf;

  PutString(&p, entry->kind);
  memcpy(p, &entry->hash, 8);
  p += 8;
  PutString(&p, entry->observed_json);
  memcpy(p, &entry->applied_at, 8);
  p += 8;
  memcpy(p, &entry->last_status, 4);
  p += 4;
  const uint8_t orphaned = entry->orphaned ? 1 : 0;
  memcpy(p, &orphaned, 1);
  p += 1;

  *out_len = size;
  return buf;
}

static inline void DecodeEntry(const uint8_t* buf, StateEntry* out) {
  const uint8_t* p = buf;
  out->kind = GetString(&p);
  memcpy(&out->hash, p, 8);
  p += 8;
  out->observed_json = GetString(&p);
  memcpy(&out->applied_at, p, 8);
  p += 8;
  memcpy(&out->last_status, p, 4);
  p += 4;
  out->orphaned = *p != 0;
}

static inline uint8_t* EncodeHistoryRecord(const HistoryRecord* rec, uint32_t* out_len) {
  const uint32_t size = StringEncodedSize(rec->kind) + StringEncodedSize(rec->hash_before) +
                        StringEncodedSize(rec->hash_after) + StringEncodedSize(rec->reason) + 4 /*action*/ +
                        4 /*status*/ + 8 /*run_id*/ + 8 /*applied_at*/;

  uint8_t* buf = (uint8_t*)malloc(size);
  uint8_t* p = buf;

  PutString(&p, rec->kind);
  PutString(&p, rec->hash_before);
  PutString(&p, rec->hash_after);
  PutString(&p, rec->reason);
  memcpy(p, &rec->action, 4);
  p += 4;
  memcpy(p, &rec->status, 4);
  p += 4;
  memcpy(p, &rec->run_id, 8);
  p += 8;
  memcpy(p, &rec->applied_at, 8);
  p += 8;

  *out_len = size;
  return buf;
}

#define MIN(a, b) (a < b ? a : b)

static inline void DecodeHistoryRecord(const char* id, const uint8_t* buf, HistoryRecord* out) {
  const uint8_t* p = buf;
  out->id = strdup(id);
  out->kind = GetString(&p);
  out->hash_before = GetString(&p);
  out->hash_after = GetString(&p);
  const char* reason = GetString(&p);
  memcpy(out->reason, reason, MIN(strlen(reason), HYPHA_REASON_MAX_LENGTH));

  memcpy(&out->action, p, 4);
  p += 4;
  memcpy(&out->status, p, 4);
  p += 4;
  memcpy(&out->run_id, p, 8);
  p += 8;
  memcpy(&out->applied_at, p, 8);
}

#endif  // HYPHA_HISTORY_H
