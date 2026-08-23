#include "state_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/annotation.h"
#include "hypha/log.h"
#include "hypha/state.h"
#include "state_log.h"

void StateEntryFree(StateEntry* entry) {
  if (!entry)
    return;

  free(entry->id);
  free(entry->kind);
  free(entry->name);
  free(entry->observed_json);
  free(entry->labels);
  free(entry->annotations);
  entry->id = entry->kind = entry->name = entry->observed_json = NULL;
  entry->labels = NULL;
  entry->annotations = NULL;
}

StateStore* StateStoreOpen(const char* path) {
  StateLog* log = StateLogOpen(path);
  if (!log)
    return NULL;

  StateStore* store = (StateStore*)malloc(sizeof(StateStore));
  store->log = log;
  IndexInit(&store->index, 64);

  StateLogReplay(log, ReplayCallback, &store->index);
  return store;
}

void StateStoreClose(StateStore* store) {
  if (!store)
    return;

  IndexFree(&store->index);
  StateLogClose(store->log);
  free(store);
}

bool StateStoreGet(StateStore* store, const char* id, StateEntry* out) {
  bool success = false;
  StateLogLocation loc;
  if (!IndexGet(&store->index, id, &loc))
    goto finished;

  uint8_t* value = NULL;
  uint32_t value_len = 0;
  if (!StateLogRead(store->log, loc, &value, &value_len))
    goto finished;

  memset(out, 0, sizeof(StateEntry));
  out->id = strdup(id);
  success = DecodeStateEntry(value, value_len, out);
  free(value);
finished:
  return success;
}

bool StateStorePut(StateStore* store, const StateEntry* entry) {
  uint8_t* value = NULL;
  size_t value_len = 0;
  EncodeStateEntry(entry, &value, &value_len);

  StateLogLocation loc;
  const bool ok = StateLogAppendPut(store->log, entry->id, value, (uint32_t)value_len, &loc);
  free(value);

  if (ok)
    IndexSet(&store->index, entry->id, loc);
  return ok;
}

bool StateStoreDelete(StateStore* store, const char* id) {
  bool success = false;
  if (!StateLogAppendTombstone(store->log, id))
    goto finished;

  IndexRemove(&store->index, id);
  success = true;
finished:
  return success;
}

bool StateStoreVisitAll(StateStore* store, StateStoreVisitFn visit, void* data) {
  for (uint32_t i = 0; i < store->index.capacity; i++) {
    IndexSlot* slot = &store->index.slots[i];
    if (IsIndexSlotEmpty(slot))
      continue;

    uint8_t* value = NULL;
    uint32_t value_len = 0;
    if (!StateLogRead(store->log, slot->loc, &value, &value_len))
      continue;

    StateEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.id = strdup(slot->key);
    const bool decoded = DecodeStateEntry(value, value_len, &entry);
    free(value);

    if (!decoded) {
      LOG_WARN("skipping unreadable state entry for '%s'", slot->key);
      StateEntryFree(&entry);
      continue;
    }

    const bool keep_going = visit(&entry, data);
    StateEntryFree(&entry);

    if (!keep_going)
      goto finished;
  }

finished:
  return true;
}

uint32_t StateStoreCount(StateStore* store) {
  return store->index.count;
}

bool StateStoreFlush(StateStore* store) {
  return StateLogFlush(store->log);
}

static inline size_t PutBit(uint8_t* ptr, const bool rhs) {
  ASSERT(ptr);
  const uint8_t orphaned = rhs ? 1 : 0;
  memcpy(ptr, &orphaned, 1);
  return 1;
}

#define DEFINE_PUT(Name, Type)                                   \
  static inline size_t Put##Name(uint8_t* ptr, const Type rhs) { \
    ASSERT(ptr);                                                 \
    const size_t total_size = sizeof(Type);                      \
    memcpy(ptr, &rhs, total_size);                               \
    return total_size;                                           \
  }

DEFINE_PUT(UInt32, uint32_t);
DEFINE_PUT(UInt64, uint64_t);
#undef DEFINE_PUT

static inline size_t PutArray(uint8_t* dst, void* src, size_t len, size_t stride) {
  ASSERT(dst);
  memcpy(dst, &len, sizeof(size_t));
  dst += sizeof(size_t);
  const size_t total_array_size = len * stride;
  if (len > 0) {
    ASSERT(src);
    memcpy(dst, src, total_array_size);
  }
  return sizeof(size_t) + total_array_size;
}

static inline size_t PutString(uint8_t* dst, const char* src) {
  return PutArray(dst, (void*)src, strlen(src), sizeof(char));
}

void EncodeStateEntry(const StateEntry* entry, uint8_t** buff, size_t* length) {
  const size_t total_len = sizeof(uint32_t) +                                              /* version */
                           StringEncodedSize(entry->kind) +                                /* kind */
                           StringEncodedSize(entry->name) +                                /* name */
                           sizeof(uint64_t) +                                              /* hash */
                           StringEncodedSize(entry->observed_json) +                       /* observed_json */
                           sizeof(time_t) +                                                /* applied_at */
                           sizeof(uint32_t) +                                              /* last_status */
                           1 +                                                             /* orphaned */
                           sizeof(size_t) + (entry->labels_len * sizeof(Label)) +          /* labels */
                           sizeof(size_t) + (entry->annotations_len * sizeof(Annotation)); /* annotations */
  uint8_t* buf = (uint8_t*)calloc(total_len, sizeof(uint8_t));
  if (!buf) {
    LOG_ERROR("failed to encode StateEntry");
    return;
  }

  uint8_t* p = &buf[0];
  p += PutUInt32(p, STATE_ENTRY_ENCODING_VERSION);
  p += PutString(p, entry->kind);
  p += PutString(p, entry->name);
  p += PutUInt64(p, entry->hash);
  p += PutString(p, entry->observed_json);
  p += PutUInt64(p, (uint64_t)entry->applied_at);
  p += PutUInt32(p, entry->last_status);
  p += PutBit(p, entry->orphaned);
  p += PutArray(p, entry->labels, entry->labels_len, sizeof(Label));
  p += PutArray(p, entry->annotations, entry->annotations_len, sizeof(Annotation));
  (*buff) = buf;
  (*length) = total_len;
}

bool DecodeStateEntry(const uint8_t* buf, size_t buf_len, StateEntry* out) {
  const uint8_t* p = buf;
  const uint8_t* end = buf + buf_len;

#define FAIL(msg)                                    \
  do {                                               \
    LOG_ERROR("failed to decode state entry: " msg); \
    StateEntryFree(out);                             \
    return false;                                    \
  } while (0)

  if (!HasBytesRemaining(p, end, sizeof(uint32_t)))
    FAIL("truncated (missing version)");
  uint32_t version = 0;
  memcpy(&version, p, sizeof(uint32_t));
  p += sizeof(uint32_t);
  if (version != STATE_ENTRY_ENCODING_VERSION)
    FAIL("unsupported encoding version -- state store likely predates a schema change and needs clearing");

  if (!GetStringChecked(&p, end, &out->kind))
    FAIL("truncated (kind)");
  if (!GetStringChecked(&p, end, &out->name))
    FAIL("truncated (name)");

  if (!HasBytesRemaining(p, end, 8))
    FAIL("truncated (hash)");
  memcpy(&out->hash, p, 8);
  p += 8;

  if (!GetStringChecked(&p, end, &out->observed_json))
    FAIL("truncated (observed_json)");

  if (!HasBytesRemaining(p, end, sizeof(time_t)))
    FAIL("truncated (applied_at)");
  memcpy(&out->applied_at, p, sizeof(time_t));
  p += sizeof(time_t);

  if (!HasBytesRemaining(p, end, 4))
    FAIL("truncated (last_status)");
  memcpy(&out->last_status, p, 4);
  p += 4;

  if (!HasBytesRemaining(p, end, 1))
    FAIL("truncated (orphaned)");
  out->orphaned = *p != 0;
  p += 1;

  if (!HasBytesRemaining(p, end, sizeof(size_t)))
    FAIL("truncated (labels_len)");
  memcpy(&out->labels_len, p, sizeof(size_t));
  p += sizeof(size_t);
  if (out->labels_len > 0) {
    if (!HasBytesRemaining(p, end, out->labels_len * sizeof(Label)))
      FAIL("truncated (labels)");
    out->labels = (Label*)malloc(out->labels_len * sizeof(Label));
    if (!out->labels)
      FAIL("allocation failure (labels)");
    memcpy(out->labels, p, out->labels_len * sizeof(Label));
    p += out->labels_len * sizeof(Label);
  } else {
    out->labels = NULL;
  }

  if (!HasBytesRemaining(p, end, sizeof(size_t)))
    FAIL("truncated (annotations_len)");
  memcpy(&out->annotations_len, p, sizeof(size_t));
  p += sizeof(size_t);
  if (out->annotations_len > 0) {
    if (!HasBytesRemaining(p, end, out->annotations_len * sizeof(Annotation)))
      FAIL("truncated (annotations)");
    out->annotations = (Annotation*)malloc(out->annotations_len * sizeof(Annotation));
    if (!out->annotations)
      FAIL("allocation failure (annotations)");
    memcpy(out->annotations, p, out->annotations_len * sizeof(Annotation));
    p += out->annotations_len * sizeof(Annotation);
  } else {
    out->annotations = NULL;
  }

#undef FAIL
  return true;
}

typedef struct {
  const char* kind;
  const char* name;
  char* found_id;
} FindIdByNameContext;

static bool VisitFindIdByName(const StateEntry* entry, void* data) {
  FindIdByNameContext* ctx = (FindIdByNameContext*)data;
  if (!entry->kind || !entry->name)
    return true;  // keep looking

  if (strcmp(entry->kind, ctx->kind) != 0 || strcmp(entry->name, ctx->name) != 0)
    return true;  // keep looking

  ctx->found_id = strdup(entry->id);
  return false;  // stop, found it
}

char* StateStoreFindIdByName(StateStore* store, const char* kind, const char* name) {
  if (!store || !kind || !name)
    return NULL;

  FindIdByNameContext ctx = {.kind = kind, .name = name, .found_id = NULL};
  StateStoreVisitAll(store, VisitFindIdByName, &ctx);
  return ctx.found_id;
}
