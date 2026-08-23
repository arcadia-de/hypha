#ifndef HYPHA_HISTORY_H
#define HYPHA_HISTORY_H

#include <uuid/uuid.h>

#include "hypha.h"
#include "hypha/annotation.h"
#include "hypha/controller.h"
#include "hypha/label.h"

typedef struct {
  char* id;
  char* kind;
  char* name;
  ControllerAction action;
  ControllerStatus status;
  uint64_t hash_before;
  uint64_t hash_after;
  Reason reason;
  uuid_t run_id;
  int64_t applied_at;

  Label* labels;
  size_t labels_len;

  Annotation* annotations;
  size_t annotations_len;
} HistoryRecord;

void HistoryRecordFree(HistoryRecord* rec);

HistoryLog* HistoryLogOpen(const char* path, uint64_t max_bytes, uint32_t keep_rotations);
void HistoryLogClose(HistoryLog* log);
bool HistoryLogAppend(HistoryLog* log, const HistoryRecord* rec);
bool HistoryLogFlush(HistoryLog* log);

typedef bool (*HistoryLogVisitFn)(const HistoryRecord* rec, void* data);
void HistoryLogReplay(HistoryLog* log, HistoryLogVisitFn fn, void* data);

#endif  // HYPHA_HISTORY_H
