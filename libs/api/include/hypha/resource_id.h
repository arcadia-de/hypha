#ifndef HYPHA_RESOURCE_ID_H
#define HYPHA_RESOURCE_ID_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <string.h>
#include <uuid/uuid.h>

typedef uuid_t ResourceId;
typedef char ResourceIdStr[48];

static inline void GenerateResourceId(ResourceId* result) {
  uuid_generate_random(*result);
}

static inline void ResourceIdCStr(const ResourceId* id, ResourceIdStr result) {
  uuid_unparse_lower(*id, result);
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RESOURCE_ID_H
