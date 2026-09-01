#ifndef HYPHA_SYMLINK_SPEC_H
#define HYPHA_SYMLINK_SPEC_H

#include <stdlib.h>

typedef struct {
  char* source;
  size_t source_len;

  char* target;
  size_t target_len;
} SymlinkSpec;

#endif  // HYPHA_SYMLINK_SPEC_H
