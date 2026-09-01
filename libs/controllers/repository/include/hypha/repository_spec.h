#ifndef HYPHA_REPOSITORY_SPEC_H
#define HYPHA_REPOSITORY_SPEC_H

#include <stddef.h>

typedef struct {
  char* url;
  size_t url_len;

  char* destination;
  size_t destination_len;
} RepositorySpec;

#endif  // HYPHA_REPOSITORY_SPEC_H
