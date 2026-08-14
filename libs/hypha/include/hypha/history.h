#ifndef HYPHA_HISTORY_H
#define HYPHA_HISTORY_H

#include "hypha.h"
#include "hypha/controller.h"

typedef struct {
  char* id;
  char* kind;
  ControllerAction action;
  ControllerStatus status;
  char* hash_before;
  char* hash_after;
  char* reason;
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

#endif  // HYPHA_HISTORY_H
