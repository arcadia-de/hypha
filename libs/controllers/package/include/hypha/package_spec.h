#ifndef HYPHA_PACKAGE_SPEC_H
#define HYPHA_PACKAGE_SPEC_H

#include <stddef.h>

typedef struct {
  char* name;
  size_t name_len;

  char* manager;
  size_t manager_len;
} PackageSpec;

#endif  // HYPHA_PACKAGE_SPEC_H
