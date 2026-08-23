#ifndef HYPHA_STATE_INDEX_H
#define HYPHA_STATE_INDEX_H

#include "hypha/state.h"
#include "state_log.h"

typedef struct {
  char* key;
  StateLogLocation loc;
} IndexSlot;

typedef struct {
  IndexSlot* slots;
  uint32_t capacity;
  uint32_t count;
} Index;

void IndexInit(Index* idx, uint32_t capacity);
void IndexFree(Index* idx);
void IndexGrow(Index* idx);
void IndexSet(Index* idx, const char* key, StateLogLocation loc);
bool IndexGet(Index* idx, const char* key, StateLogLocation* out_loc);
void IndexRemove(Index* idx, const char* key);

// True when the slot holds neither a live key nor a tombstone (i.e. it has never been used,
// or a resize just reset it) -- callers scanning all slots should skip these.
bool IsIndexSlotEmpty(IndexSlot* rhs);

#endif  // HYPHA_STATE_INDEX_H
