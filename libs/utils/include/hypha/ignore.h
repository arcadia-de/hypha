#ifndef HYPHA_IGNORE_H
#define HYPHA_IGNORE_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdlib.h>

typedef struct _Ignore Ignore;

Ignore* NewIgnore(const char* filename);
void AppendIgnorePatterns(Ignore* dst, char** patterns, const size_t len);
void AppendIgnoreFile(Ignore* dst, char* src);
void AppendIgnore(Ignore* dst, Ignore* src);
bool IgnoreIsEmpty(Ignore* rhs);
bool IgnoreMatches(Ignore* ig, const char* path);
void FreeIgnore(Ignore* rhs);

static inline void AppendIgnorePattern(Ignore* ignore, char* pattern) {
  if (!ignore || !pattern)
    return;
  return AppendIgnorePatterns(ignore, &pattern, 1);
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_IGNORE_H
