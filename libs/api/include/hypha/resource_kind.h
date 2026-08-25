#ifndef HYPHA_RESOURCE_KIND_H
#define HYPHA_RESOURCE_KIND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef int64_t ResourceKind;
static const ResourceKind kInvalidResourceKind = -1;

typedef struct {
  ResourceKind kind;
  char* name;
} ResourceKindInfo;

ResourceKind NewResourceKind(const char* name);
ResourceKind FindResourceKind(const char* name);
ResourceKind FindOrCreateResourceKind(const char* name);
ResourceKindInfo* GetResourceKindInfo(const ResourceKind rhs);
ResourceKindInfo* FindResourceKindInfo(const char* name);
const char* FindResourceKindName(const ResourceKind rhs);
uint64_t GetTotalNumberOfResourceKinds();

typedef bool (*VisitResourceKindFn)(const ResourceKindInfo* kind, void* data);
void VisitAllResourceKinds(VisitResourceKindFn fn, void* data);

void FreeAllResourceKinds();

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RESOURCE_KIND_H
