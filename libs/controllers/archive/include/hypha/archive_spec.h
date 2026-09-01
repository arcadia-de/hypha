#ifndef HYPHA_ARCHIVE_SPEC_H
#define HYPHA_ARCHIVE_SPEC_H

#include <stddef.h>

typedef struct {
  char* source;
  size_t source_len;

  char* destination;
  size_t destination_len;
} ArchiveSpec;

#endif  // HYPHA_ARCHIVE_SPEC_H
