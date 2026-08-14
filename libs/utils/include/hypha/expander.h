#ifndef HYPHA_EXPANDER_H
#define HYPHA_EXPANDER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)
typedef const char* (*ExpanderResolveFn)(const char symbol, void* data);

typedef struct {
  ExpanderResolveFn resolve;

  void* data;
  void (*free_data)(void*);
} Expander;

bool Expand(Expander* expander, const char* value, const size_t value_len, char** result, size_t* result_len);

static inline bool ExpandStr(Expander* expander, const char* value, char** result, size_t* result_len) {
  return Expand(expander, value, strlen(value), result, result_len);
}

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_EXPANDER_H
