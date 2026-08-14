#ifndef HYPHA_GLOB_H
#define HYPHA_GLOB_H

#include <stddef.h>

typedef struct {
  char** paths;
  size_t paths_len;
  size_t paths_cap;
} Glob;

bool GlobFiles(const char* dir, const char* pattern, Glob* glob, const bool recursive);

#endif  // HYPHA_GLOB_H
