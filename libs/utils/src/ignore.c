#define _DEFAULT_SOURCE
#include "hypha/ignore.h"

#include "hypha/platform.h"

#if defined(HYPHA_LINUX) || defined(HYPHA_OSX)
#include <fnmatch.h>
#define PLATFORM_MATCH(file, pattern) (fnmatch(pattern, file, FNM_CASEFOLD) == 0)
#else
#error "Unsupporte Oprating System"
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "hypha/assertions.h"

#ifndef FNM_CASEFOLD
#ifdef FNM_IGNORECASE
#define FNM_CASEFOLD FNM_IGNORECASE
#else
#define FNM_CASEFOLD 0
#endif  // FNM_IGNORECASE
#endif  // FNM_CASEFOLD

#define MAX_LINE 256

struct _Ignore {
  char** patterns;
  size_t patterns_len;
  size_t patterns_cap;
};

void Normalize(char* path) {
  while (*path) {
    if (*path == '\\')
      *path = '/';
    path++;
  }
}

typedef bool (*ParseIgnoreFileCallbackFn)(char* pattern, void*);

static inline void ParseIgnoreFile(const char* filename, ParseIgnoreFileCallbackFn fn, void* data) {
  if (!filename)
    return;

  FILE* file = fopen(filename, "r");
  if (!file)
    return;

  char line[MAX_LINE];
  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0' || line[0] == '#')
      continue;

    Normalize(line);

    char pattern[MAX_LINE];
    char *src = line, *dst = pattern;
    while (*src) {
      if (*src == '*' && *(src + 1) == '*') {
        *dst++ = '*';
        src += 2;
      } else {
        *dst++ = *src++;
      }
    }
    *dst = '\0';

    if (!fn(pattern, data))
      goto finished;
  }

finished:
  fclose(file);
}

static inline void EnsureCap(Ignore* ig, const size_t new_len) {
  ASSERT(ig);
  if ((ig->patterns_len + new_len) < ig->patterns_cap)
    return;

  const size_t new_cap = ig->patterns_cap + new_len + 1;
  const size_t total_size = sizeof(char*) * new_cap;
  char** new_patterns = (char**)realloc(ig->patterns, total_size);
  if (!new_patterns)
    return;

  ig->patterns = new_patterns;
  ig->patterns_cap = new_cap;
}

void AppendIgnorePatterns(Ignore* ignore, char** patterns, const size_t len) {
  if (!ignore)
    return;

  EnsureCap(ignore, ignore->patterns_len + len);
  for (size_t i = 0; i < len; i++)
    ignore->patterns[ignore->patterns_len + i] = strdup(patterns[i]);

  ignore->patterns_len += len;
}

static inline bool OnPattern(char* pattern, void* data) {
  Ignore* ignore = (Ignore*)data;
  AppendIgnorePattern(ignore, pattern);
  return true;
}

void AppendIgnoreFile(Ignore* dst, char* src) {
  if (!dst || !src)
    return;

  return ParseIgnoreFile(src, &OnPattern, dst);
}

void AppendIgnore(Ignore* dst, Ignore* src) {
  if (!dst || !src || src->patterns_len == 0)
    return;

  return AppendIgnorePatterns(dst, src->patterns, src->patterns_len);
}

static const size_t kInitCap = 8;

Ignore* NewIgnore(const char* filename) {
  Ignore* ignore = (Ignore*)malloc(sizeof(Ignore));
  if (ignore) {
    memset(ignore, 0, sizeof(Ignore));
    EnsureCap(ignore, kInitCap);
    if (filename)
      ParseIgnoreFile(filename, &OnPattern, ignore);
  }

  return ignore;
}

bool IgnoreMatches(Ignore* ig, const char* path) {
  if (!ig || !path || ig->patterns_len == 0)
    return false;

  for (size_t i = 0; i < ig->patterns_len; i++) {
    if (fnmatch(ig->patterns[i], path, FNM_CASEFOLD) == 0)
      return true;
  }

  return false;
}

void FreeIgnore(Ignore* rhs) {
  if (!rhs)
    return;

  if (rhs->patterns) {
    for (size_t i = 0; i < rhs->patterns_len; i++)
      free(rhs->patterns[i]);
    free(rhs->patterns);
  }

  free(rhs);
}

bool IgnoreIsEmpty(Ignore* rhs) {
  return !rhs || !rhs->patterns || rhs->patterns_len == 0;
}
