#ifndef HYPHA_NAME_H
#define HYPHA_NAME_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <string.h>

#include "hypha/resource_kind.h"

#ifndef HYPHA_NAME_MAX_SIZE
#define HYPHA_NAME_MAX_SIZE 128
#endif  // HYPHA_NAME_MAX_SIZE

typedef char Name[HYPHA_NAME_MAX_SIZE];

static inline int CompareName(const Name lhs, const Name rhs) {
  return strncmp(lhs, rhs, HYPHA_NAME_MAX_SIZE);
}

static inline bool NameEq(const Name lhs, const Name rhs) {
  return CompareName(lhs, rhs) == 0;
}

void GenerateDefaultResourceName(ResourceKind kind, Name* rhs);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_NAME_H
