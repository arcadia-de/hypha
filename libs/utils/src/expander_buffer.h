#ifndef HYPHA_EXPANDER_BUFFER_H
#define HYPHA_EXPANDER_BUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char* data;
  size_t len;
  size_t cap;
} ExpanderBuffer;

static inline void InitExBuffer(ExpanderBuffer* buff, size_t init_cap) {
  memset(buff, 0, sizeof(ExpanderBuffer));
  if (init_cap == 0)
    init_cap = 128;
  const size_t total_cap = sizeof(char) * init_cap;
  char* data = (char*)malloc(total_cap);
  if (!data)
    return;
  memset(data, 0, total_cap);
  data[0] = '\0';
  buff->data = data;
  buff->len = 0;
  buff->cap = init_cap;
}

static inline void FreeExBuff(ExpanderBuffer* buff) {
  if (!buff)
    return;
  if (buff->data)
    free(buff->data);
}

static inline void ExBuffReserve(ExpanderBuffer* buff, const size_t new_len) {
  if (!buff || new_len <= buff->cap)
    return;

  size_t new_cap = buff->cap * 2;
  while (new_cap < new_len)
    new_cap *= 2;

  char* new_data = (char*)realloc(buff->data, new_cap);
  if (!new_data)
    return;

  buff->data = new_data;
  buff->cap = new_cap;
}

static inline void ExBuffAppend(ExpanderBuffer* buff, const char* value, const size_t value_len) {
  if (!buff || !value || value_len == 0)
    return;

  ExBuffReserve(buff, buff->len + value_len);
  memcpy(&buff->data[buff->len], value, value_len);
  buff->len += value_len;
  buff->data[buff->len] = '\0';
}

static inline void ExBuffAppendStr(ExpanderBuffer* buff, const char* value) {
  return ExBuffAppend(buff, value, strlen(value));
}

static inline void ExBuffAppendChar(ExpanderBuffer* buff, const char c) {
  if (!buff)
    return;

  ExBuffReserve(buff, buff->len + 1);
  buff->data[buff->len] = c;
  buff->len += 1;
  buff->data[buff->len] = '\0';
}

#endif  // HYPHA_EXPANDER_BUFFER_H
