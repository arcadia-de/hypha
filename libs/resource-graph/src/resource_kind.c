#include "hypha/resource_kind.h"

#include <stdlib.h>
#include <string.h>

#include "hypha/assertions.h"

static const size_t kInitCap = 16;
static ResourceKindInfo* kinds = NULL;
static size_t kinds_len = 0;
static size_t kinds_cap = 0;

static inline bool EnsureCapacity(const size_t new_len) {
  if (new_len == 0 || new_len < kinds_cap)
    return false;

  const size_t new_cap = (kinds_cap + new_len) * 2;
  const size_t total_size = sizeof(ResourceKindInfo) * new_cap;
  ResourceKindInfo* new_info = (ResourceKindInfo*)realloc(kinds, total_size);
  if (!new_info)
    return false;

  kinds = new_info;
  kinds_cap = new_cap;
  return true;
}

ResourceKind NewResourceKind(const char* name) {
  ASSERT(name);
  if (kinds == NULL) {
    const size_t total_size = sizeof(ResourceKindInfo) * kInitCap;
    ResourceKindInfo* new_kinds = (ResourceKindInfo*)malloc(total_size);
    if (!new_kinds)
      return kInvalidResourceKind;

    memset(new_kinds, 0, total_size);
    kinds = new_kinds;
    kinds_len = 0;
    kinds_cap = kInitCap;
  }

  EnsureCapacity(kinds_len + 1);
  ResourceKindInfo* new_info = &kinds[kinds_len];

  new_info->kind = (ResourceKind)kinds_len;
  new_info->name = strdup(name);
  ASSERT(new_info->name);

  kinds_len++;
  return new_info->kind;
}

uint64_t GetTotalNumberOfResourceKinds() {
  return kinds_len;
}

ResourceKind FindResourceKind(const char* name) {
  if (!name || !kinds || kinds_len == 0)
    return kInvalidResourceKind;

  for (size_t i = 0; i < kinds_len; i++) {
    ResourceKindInfo* info = &kinds[i];
    if (strcmp(info->name, name) == 0)
      return info->kind;
  }

  return kInvalidResourceKind;
}

ResourceKindInfo* GetResourceKindInfo(const ResourceKind rhs) {
  if (rhs == kInvalidResourceKind || !kinds || kinds_len == 0)
    return NULL;

  for (size_t i = 0; i < kinds_len; i++) {
    ResourceKindInfo* info = &kinds[i];
    if (info->kind == rhs)
      return info;
  }

  return NULL;
}

ResourceKindInfo* FindResourceKindInfo(const char* name) {
  if (!name || !kinds || kinds_len == 0)
    return NULL;

  for (size_t i = 0; i < kinds_len; i++) {
    ResourceKindInfo* info = &kinds[i];
    if (strcmp(info->name, name) == 0)
      return info;
  }

  return NULL;
}

void VisitAllResourceKinds(VisitResourceKindFn fn, void* data) {
  if (!fn || !kinds || kinds_len == 0)
    return;

  for (size_t i = 0; i < kinds_len; i++) {
    if (!fn(&kinds[i], data))
      return;
  }
}

static inline void FreeResourceKindInfo(ResourceKindInfo* info) {
  ASSERT(info);
  if (info->name)
    free(info->name);
}

void FreeAllResourceKinds() {
  if (!kinds || kinds_len == 0)
    return;

  for (size_t i = 0; i < kinds_len; i++) {
    ResourceKindInfo* info = &kinds[i];
    ASSERT(info);
    FreeResourceKindInfo(info);
  }

  free(kinds);
}
