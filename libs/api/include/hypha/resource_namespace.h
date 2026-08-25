#ifndef HYPHA_RESOURCE_NAMESPACE_H
#define HYPHA_RESOURCE_NAMESPACE_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>
#include <string.h>

#ifndef HYPHA_RESOURCE_NAMESPACE_MAX_SIZE
#define HYPHA_RESOURCE_NAMESPACE_MAX_SIZE 64
#endif  // HYPHA_RESOURCE_NAMESPACE_MAX_SIZE

typedef char ResourceNamespace[HYPHA_RESOURCE_NAMESPACE_MAX_SIZE];

static const ResourceNamespace kDefaultResourceNamespace = "";
static const ResourceNamespace kCoreResourceNamespace = "hypha";

static inline void SetResourceNamespace(ResourceNamespace ns, const char* value) {
  memset(ns, 0, sizeof(ResourceNamespace));
  if (value)
    strncpy(ns, value, sizeof(ResourceNamespace) - 1);
}

static inline int CompareResourceNamespace(const ResourceNamespace lhs, const ResourceNamespace rhs) {
  return strncmp(lhs, rhs, HYPHA_RESOURCE_NAMESPACE_MAX_SIZE);
}

static inline bool ResourceNamespaceEq(const ResourceNamespace lhs, const ResourceNamespace rhs) {
  return CompareResourceNamespace(lhs, rhs) == 0;
}

static inline bool IsDefaultResourceNamespace(const ResourceNamespace ns) {
  return ns[0] == '\0';
}

static inline bool IsReservedResourceNamespace(const ResourceNamespace ns) {
  return ResourceNamespaceEq(ns, kCoreResourceNamespace);
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RESOURCE_NAMESPACE_H
