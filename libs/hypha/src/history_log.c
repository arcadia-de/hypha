#include "history_log.h"

#include <linux/limits.h>
#include <stdio.h>

void HistoryRecordFree(HistoryRecord* rec) {
  if (!rec)
    return;

  free(rec->id);
  free(rec->kind);
  free(rec->name);
  free(rec->labels);
  free(rec->annotations);
  rec->id = rec->kind = rec->name = NULL;
  rec->labels = NULL;
  rec->annotations = NULL;
}

static inline void RotatedPath(const char* path, uint32_t n, char* out, size_t out_len) {
  snprintf(out, out_len, "%s.%u", path, n);
}

static bool RotateHistoryLog(HistoryLog* log) {
  StateLogClose(log->log);
  log->log = NULL;

  if (log->keep_rotations == 0) {
    remove(log->path);
  } else {
    char from[PATH_MAX], to[PATH_MAX];
    for (uint32_t n = log->keep_rotations - 1; n >= 1; n--) {
      RotatedPath(log->path, n, from, sizeof from);
      RotatedPath(log->path, n + 1, to, sizeof to);
      rename(from, to);  // ENOENT (nothing at this generation yet) is fine to ignore
    }
    RotatedPath(log->path, 1, to, sizeof to);
    rename(log->path, to);
  }

  log->log = StateLogOpen(log->path);
  return log->log != NULL;
}

HistoryLog* HistoryLogOpen(const char* path, uint64_t max_bytes, uint32_t keep_rotations) {
  StateLog* raw = StateLogOpen(path);
  if (!raw)
    return NULL;  // NOLINT(modernize-use-nullptr)

  HistoryLog* hlog = (HistoryLog*)malloc(sizeof(HistoryLog));
  if (!hlog) {
    StateLogClose(raw);
    return NULL;  // NOLINT(modernize-use-nullptr)
  }

  hlog->log = raw;
  hlog->path = strdup(path);
  hlog->max_bytes = max_bytes;
  hlog->keep_rotations = keep_rotations;
  return hlog;
}

void HistoryLogClose(HistoryLog* log) {
  if (!log)
    return;

  StateLogClose(log->log);
  free(log->path);
  free(log);
}

bool HistoryLogAppend(HistoryLog* log, const HistoryRecord* rec) {
  uint32_t value_len = 0;
  uint8_t* value = EncodeHistoryRecord(rec, &value_len);

  const bool ok = StateLogAppendPut(log->log, rec->id, value, value_len, NULL);
  free(value);

  if (ok && log->max_bytes > 0 && StateLogSizeBytes(log->log) >= log->max_bytes)
    RotateHistoryLog(log);

  return ok;
}

bool HistoryLogFlush(HistoryLog* log) {
  return StateLogFlush(log->log);
}

typedef struct {
  HistoryLogVisitFn visit;
  void* data;
} ReplayForward;

static inline bool ForwardToVisitor(const char* key, bool tombstone, const uint8_t* value, uint32_t value_len,
                                    void* data) {
  if (tombstone)
    return true;

  ReplayForward* fwd = (ReplayForward*)data;

  HistoryRecord rec;
  if (!DecodeHistoryRecord(key, value, value_len, &rec)) {
    LOG_WARN("skipping unreadable history record for: %s", key);
    return true;  // keep replaying the rest of the log rather than aborting on one bad record
  }

  const bool keep_going = fwd->visit(&rec, fwd->data);
  HistoryRecordFree(&rec);
  return keep_going;
}

void HistoryLogReplay(HistoryLog* log, HistoryLogVisitFn fn, void* data) {
  ReplayForward fwd = {.visit = fn, .data = data};
  StateLogReplayWithValues(log->log, ForwardToVisitor, &fwd);
}
