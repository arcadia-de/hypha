#ifndef HYPHA_STRUCTURED_LOG_H
#define HYPHA_STRUCTURED_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdlib.h>

#include "hypha/assertions.h"

#define DECLARE_STRUCTURED_LOG(Type)                                       \
  typedef struct {                                                         \
    Type* data;                                                            \
    size_t data_len;                                                       \
    size_t data_cap;                                                       \
  } Type##Log;                                                             \
  void Init##Type##Log(Type##Log* log, const size_t init_cap);             \
  Type* New##Type(Type##Log* log);                                         \
  void Append##Type##s(Type##Log* dst, Type* src, const size_t src_len);   \
  void Free##Type##Log(Type##Log* log);                                    \
  typedef bool (*Visit##Type##Fn)(uint64_t, const Type*, void*);           \
  void VisitAll##Type##s(Type##Log* dlog, Visit##Type##Fn fn, void* data); \
  static inline bool Is##Type##LogEmpty(Type##Log* rhs) {                  \
    if (rhs == NULL)                                                       \
      return true;                                                         \
    return rhs->data == NULL || rhs->data_len == 0;                        \
  }                                                                        \
  static inline void Append##Type(Type##Log* dst, Type* src) {             \
    ASSERT(dst);                                                           \
    ASSERT(src);                                                           \
    return Append##Type##s(dst, src, 1);                                   \
  }                                                                        \
  static inline void Append##Type##Log(Type##Log* dst, Type##Log* src) {   \
    ASSERT(dst);                                                           \
    ASSERT(src);                                                           \
    if (Is##Type##LogEmpty(src))                                           \
      return;                                                              \
    return Append##Type##s(dst, src->data, src->data_len);                 \
  }

#define DEFINE_STRUCTURED_LOG(Type)                                         \
  void Init##Type##Log(Type##Log* log, const size_t init_cap) {             \
    ASSERT(log);                                                            \
    ASSERT(init_cap > 0);                                                   \
    const size_t total_size = sizeof(Type) * init_cap;                      \
    Type* data = (Type*)malloc(total_size);                                 \
    if (!data) {                                                            \
      LOG_ERROR("failed to allocate data of cap %zu", init_cap);            \
      return;                                                               \
    }                                                                       \
    memset(data, 0, total_size);                                            \
    log->data = data;                                                       \
    log->data_len = 0;                                                      \
    log->data_cap = init_cap;                                               \
  }                                                                         \
  static inline bool EnsureCap(Type##Log* log, const size_t new_len) {      \
    if (new_len < log->data_cap)                                            \
      return true;                                                          \
    const size_t new_cap = log->data_cap + new_len;                         \
    const size_t total_size = sizeof(Type) * new_cap;                       \
    Type* new_data = (Type*)realloc(log->data, total_size);                 \
    if (!new_data)                                                          \
      return false;                                                         \
    log->data = new_data;                                                   \
    log->data_cap = new_cap;                                                \
    return true;                                                            \
  }                                                                         \
  Type* New##Type(Type##Log* log) {                                         \
    ASSERT(log);                                                            \
    EnsureCap(log, log->data_len + 1);                                      \
    Type* data = &log->data[log->data_len];                                 \
    log->data_len++;                                                        \
    memset(data, 0, sizeof(Type));                                          \
    return data;                                                            \
  }                                                                         \
  void Append##Type##s(Type##Log* dst, Type* data, const size_t num_data) { \
    ASSERT(dst);                                                            \
    ASSERT(data);                                                           \
    ASSERT(num_data > 0);                                                   \
    EnsureCap(dst, dst->data_len + num_data);                               \
    const size_t total_size = sizeof(Type) * num_data;                      \
    memcpy(&dst->data[dst->data_len], data, total_size);                    \
    dst->data_len += num_data;                                              \
  }                                                                         \
  void VisitAll##Type##s(Type##Log* log, Visit##Type##Fn fn, void* data) {  \
    if (Is##Type##LogEmpty(log))                                            \
      return;                                                               \
    for (size_t i = 0; i < log->data_len; i++) {                            \
      const Type* item = &log->data[i];                                     \
      if (!fn((uint64_t)i, item, data))                                     \
        return;                                                             \
    }                                                                       \
  }                                                                         \
  void Free##Type##Log(Type##Log* log) {                                    \
    if (!log)                                                               \
      return;                                                               \
    if (log->data) {                                                        \
      for (size_t i = 0; i < log->data_len; i++)                            \
        Free##Type(&log->data[i]);                                          \
      free(log->data);                                                      \
    }                                                                       \
    log->data = NULL;                                                       \
    log->data_len = log->data_cap = 0;                                      \
  }

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_STRUCTURED_LOG_H
