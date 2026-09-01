#include "state_log.h"

#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hypha/crc32.h"

struct _StateLog {
  FILE* file;
  char* path;
};

static inline FILE* OpenAppendOnly(const char* p) {
  return fopen(p, "a+b");
}

StateLog* StateLogOpen(const char* path) {
  StateLog* log = NULL;
  if (!path)
    goto finished;

  FILE* f = OpenAppendOnly(path);
  if (!f)
    goto finished;

  log = (StateLog*)malloc(sizeof(StateLog));
  if (log) {
    log->file = f;
    log->path = strdup(path);
  }

finished:
  return log;
}

void StateLogClose(StateLog* log) {
  if (!log)
    return;

  if (log->file)
    fclose(log->file);

  free(log->path);
  free(log);
}

static inline void EncodeRecord(bool tombstone, const char* key, const uint8_t* value, uint32_t value_len,
                                uint8_t** out_body, uint32_t* out_body_len) {
  const uint32_t key_len = (uint32_t)strlen(key);
  const uint32_t body_len = 1 + 4 + key_len + (tombstone ? 0 : value_len);

  uint8_t* body = (uint8_t*)malloc(body_len);
  uint8_t* p = body;

  const uint8_t tomb_byte = tombstone ? 1 : 0;
  memcpy(p, &tomb_byte, 1);
  p += 1;
  memcpy(p, &key_len, 4);
  p += 4;
  memcpy(p, key, key_len);
  p += key_len;
  if (!tombstone) {
    memcpy(p, value, value_len);
    p += value_len;
  }

  (*out_body_len) = body_len;
  (*out_body) = body;
}

static inline bool AppendFramedRecord(StateLog* log, const uint8_t* body, uint32_t body_len,
                                      uint32_t value_offset_in_body, uint32_t value_len, StateLogLocation* out_loc) {
  bool success = false;
  const uint32_t crc = HyphaCrc32C(body, body_len);

  if (fseek(log->file, 0, SEEK_END) != 0)
    goto finished;

  const long record_start = ftell(log->file);
  if (record_start < 0)
    goto finished;

  if (fwrite(&body_len, 4, 1, log->file) != 1)
    goto finished;
  if (fwrite(&crc, 4, 1, log->file) != 1)
    goto finished;
  if (fwrite(body, 1, body_len, log->file) != body_len)
    goto finished;

  if (fflush(log->file) != 0)
    goto finished;

  if (out_loc) {
    out_loc->offset = (uint64_t)record_start + 4 + 4 + value_offset_in_body;
    out_loc->length = value_len;
  }

  success = true;
finished:
  return success;
}

bool StateLogAppendPut(StateLog* log, const char* key, const uint8_t* value, uint32_t value_len,
                       StateLogLocation* out_loc) {
  uint8_t* body = NULL;
  uint32_t body_len = 0;
  EncodeRecord(false, key, value, value_len, &body, &body_len);

  const uint32_t value_offset_in_body = body_len - value_len;
  const bool ok = AppendFramedRecord(log, body, body_len, value_offset_in_body, value_len, out_loc);

  free(body);
  return ok;
}

bool StateLogAppendTombstone(StateLog* log, const char* key) {
  uint8_t* body = NULL;
  uint32_t body_len = 0;
  EncodeRecord(true, key, NULL, 0, &body, &body_len);

  const bool ok = AppendFramedRecord(log, body, body_len, body_len, 0, NULL);

  free(body);
  return ok;
}

bool StateLogRead(StateLog* log, StateLogLocation loc, uint8_t** out_value, uint32_t* out_len) {
  bool success = false;

  if (loc.length == 0) {
    *out_value = NULL;
    *out_len = 0;
    goto successful;
  }

  if (fseek(log->file, (long)loc.offset, SEEK_SET) != 0)
    goto finished;

  uint8_t* value = (uint8_t*)malloc(loc.length);
  if (fread(value, 1, loc.length, log->file) != loc.length) {
    free(value);
    goto finished;
  }

  *out_value = value;
  *out_len = loc.length;
successful:
  success = true;
finished:
  return success;
}

void StateLogReplay(StateLog* log, StateLogReplayFn fn, void* data) {
  if (fseek(log->file, 0, SEEK_SET) != 0)
    return;

  for (;;) {
    const long record_start = ftell(log->file);

    uint32_t body_len = 0, stored_crc = 0;
    if (fread(&body_len, 4, 1, log->file) != 1)
      break;

    if (fread(&stored_crc, 4, 1, log->file) != 1)
      break;

    uint8_t* body = (uint8_t*)malloc(body_len);
    const size_t got = fread(body, 1, body_len, log->file);
    if (got != body_len) {
      free(body);
      break;
    }

    if (HyphaCrc32C(body, body_len) != stored_crc) {
      free(body);
      break;
    }

    const uint8_t tombstone = body[0];
    uint32_t key_len = 0;
    memcpy(&key_len, body + 1, 4);
    char* key = (char*)malloc(key_len + 1);
    memcpy(key, body + 5, key_len);
    key[key_len] = '\0';

    StateLogLocation loc = {
        .offset = (uint64_t)record_start + 8 + 5 + key_len,
        .length = tombstone ? 0 : (body_len - 5 - key_len),
    };

    const bool keep_going = fn(key, tombstone != 0, loc, data);
    free(key);
    free(body);

    if (!keep_going)
      return;
  }
}

bool StateLogFlush(StateLog* log) {
  if (fflush(log->file) != 0)
    return false;

  return fsync(fileno(log->file)) == 0;
}

static inline void CreateTempStatePath(const char* prefix, char* p, const size_t plen) {
  snprintf(p, plen, "%s.compact.tmp", prefix);
}

bool StateLogCompact(StateLog* log, const StateLogKeepEntry* keep, uint32_t keep_count) {
  char tmp_path[PATH_MAX];
  CreateTempStatePath(log->path, tmp_path, PATH_MAX);

  StateLog* tmp = StateLogOpen(tmp_path);
  if (!tmp)
    goto failed;

  for (uint32_t i = 0; i < keep_count; i++) {
    if (!StateLogAppendPut(tmp, keep[i].key, keep[i].value, keep[i].value_len, NULL)) {
      StateLogClose(tmp);
      remove(tmp_path);
      goto failed;
    }
  }

  StateLogClose(tmp);

  fclose(log->file);
  if (rename(tmp_path, log->path) != 0) {
    log->file = OpenAppendOnly(log->path);
    goto failed;
  }

  log->file = OpenAppendOnly(log->path);
  return log->file != NULL;
failed:
  return false;
}

uint64_t StateLogSizeBytes(StateLog* log) {
  if (fseek(log->file, 0, SEEK_END) != 0)
    return 0;

  const long size = ftell(log->file);
  return size < 0 ? 0 : (uint64_t)size;
}

void StateLogReplayWithValues(StateLog* log, StateLogReplayValueFn fn, void* data) {
  if (fseek(log->file, 0, SEEK_SET) != 0)
    return;

  for (;;) {
    uint32_t body_len = 0, stored_crc = 0;
    if (fread(&body_len, 4, 1, log->file) != 1)
      break;

    if (fread(&stored_crc, 4, 1, log->file) != 1)
      break;

    uint8_t* body = (uint8_t*)malloc(body_len);
    const size_t got = fread(body, 1, body_len, log->file);
    if (got != body_len) {
      free(body);
      break;
    }

    if (HyphaCrc32C(body, body_len) != stored_crc) {
      free(body);
      break;
    }

    const uint8_t tombstone = body[0];
    uint32_t key_len = 0;
    memcpy(&key_len, body + 1, 4);
    char* key = (char*)malloc(key_len + 1);
    memcpy(key, body + 5, key_len);
    key[key_len] = '\0';

    const uint8_t* value = tombstone ? NULL : (body + 5 + key_len);
    const uint32_t value_len = tombstone ? 0 : (body_len - 5 - key_len);

    const bool keep_going = fn(key, tombstone != 0, value, value_len, data);
    free(key);
    free(body);

    if (!keep_going)
      return;
  }
}
