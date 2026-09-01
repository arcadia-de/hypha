#ifndef HYPHA_DIRECTORY_SPEC_H
#define HYPHA_DIRECTORY_SPEC_H

#include <stddef.h>

typedef struct {
  char* target;
  size_t target_len;
} DirectorySpec;

#endif  // HYPHA_DIRECTORY_SPEC_H
