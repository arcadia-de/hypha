#ifndef HYPHA_NAME_H
#define HYPHA_NAME_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <string.h>

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

// Generates a default resource name of the form "<lowercased-kind>-<random>" (e.g.
// "symlink-a3f9k2q") for resources the user didn't give an explicit name. The random
// postfix is drawn from a small lowercase alphanumeric alphabet, nanoid-style, rather
// than a full uuid, since it only needs to disambiguate siblings of the same kind, not
// serve as a stable identity on its own (that's what ResourceId/uuid is for).
// Returns a newly-allocated string the caller owns, or NULL if kind is NULL.
char* GenerateDefaultResourceName(const char* kind);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_NAME_H
