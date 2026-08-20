#ifndef HYPHA_STATE_H
#define HYPHA_STATE_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <time.h>

#include "hypha.h"

typedef struct _StateStore StateStore;
typedef struct _StateLog StateLog;

typedef struct {
  bool orphaned;
  char* id;
  char* kind;
  uint64_t hash;
  char* observed_json;
  int last_status;
  time_t applied_at;
} StateEntry;

StateStore* StateStoreOpen(const char* path);
uint32_t StateStoreCount(StateStore* store);
void StateEntryFree(StateEntry* entry);
void StateStoreClose(StateStore* store);
bool StateStoreGet(StateStore* store, const char* id, StateEntry* out);
bool StateStorePut(StateStore* store, const StateEntry* entry);
bool StateStoreDelete(StateStore* store, const char* id);
bool StateStoreCompact(StateStore* store);
bool StateStoreFlush(StateStore* store);
typedef bool (*StateStoreVisitFn)(const StateEntry* entry, void* data);
bool StateStoreVisitAll(StateStore* store, StateStoreVisitFn visit, void* data);

void DecodeStateEntry(const uint8_t* buf, StateEntry* out);
void EncodeStateEntry(const StateEntry* entry, uint8_t** buff, size_t* len);

typedef struct {
  uint64_t offset;
  uint32_t length;
} StateLogLocation;

typedef struct {
  const char* key;
  const uint8_t* value;
  uint32_t value_len;
} StateLogKeepEntry;

StateLog* StateLogOpen(const char* path);
bool StateLogAppendPut(StateLog* log, const char* key, const uint8_t* value, uint32_t value_len,
                       StateLogLocation* out_loc);
bool StateLogAppendPut(StateLog* log, const char* key, const uint8_t* value, uint32_t value_len,
                       StateLogLocation* out_loc);
bool StateLogRead(StateLog* log, StateLogLocation loc, uint8_t** out_value, uint32_t* out_len);
bool StateLogCompact(StateLog* log, const StateLogKeepEntry* keep, uint32_t keep_count);

typedef bool (*StateLogReplayFn)(const char* key, bool tombstone, StateLogLocation loc, void* data);
void StateLogReplay(StateLog* log, StateLogReplayFn fn, void* data);
void StateLogClose(StateLog* log);
bool StateLogAppendTombstone(StateLog* log, const char* key);
bool StateLogFlush(StateLog* log);

uint64_t StateLogSizeBytes(StateLog* log);

typedef bool (*StateLogReplayValueFn)(const char* key, bool tombstone, const uint8_t* value, uint32_t value_len,
                                      void* data);
void StateLogReplayWithValues(StateLog* log, StateLogReplayValueFn fn, void* data);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_STATE_H
