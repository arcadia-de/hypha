#ifndef HYPHA_PACKAGE_MANAGER_SPEC_H
#define HYPHA_PACKAGE_MANAGER_SPEC_H

#include <stddef.h>

typedef struct {
  char* type;
  size_t type_len;
} PackageManagerSpec;

#endif  // HYPHA_PACKAGE_MANAGER_SPEC_H
