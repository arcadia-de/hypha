#ifndef HYPHA_RESOURCE_SELECTOR_H
#define HYPHA_RESOURCE_SELECTOR_H

#include "hypha/assertions.h"
#include "hypha/resource.h"

typedef bool (*ResourceSelectorFn)(const Resource*, void* data);

typedef struct _ResourceSelector ResourceSelector;

ResourceSelector* NewResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*));
ResourceSelector* NewNotResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*));
ResourceSelector* NewNegateResourceSelector(ResourceSelector* selector);
ResourceSelector* NewAndResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
ResourceSelector* NewOrResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
ResourceSelector* NewRefResourceSelector(const char* rhs);
ResourceSelector* NewKindResourceSelector(const char* rhs);
ResourceSelector* NewLabelResourceSelector(const Label* rhs);
ResourceSelector* NewNamespaceResourceSelector(const char* ns);
ResourceSelector* NewNameResourceSelector(const char* name);
ResourceSelector* NewStateResourceSelector(ResourceState rh);
ResourceSelector* NewAnnotationResourceSelector(const Annotation* rhs);
ResourceSelector* NewAnnotationKeyResourceSelector(const AnnotationKey* rhs);
ResourceSelector* NewAnnotationValueResourceSelector(const AnnotationValue* rhs);
bool ResourceSelectorMatch(const ResourceSelector* rs, const Resource* res);
void FreeResourceSelector(ResourceSelector* rs);

typedef struct {
  ResourceSelector** selectors;
  size_t selectors_len;
  size_t selectors_cap;
} ResourceSelectorBuilder;

void InitResourceSelectorBuilder(ResourceSelectorBuilder* builder, const size_t init_cap);
void AppendResourceSelectors(ResourceSelectorBuilder* dst, ResourceSelector** src, const size_t src_len);
ResourceSelector* BuildAndResourceSelector(ResourceSelectorBuilder* builder);
ResourceSelector* BuildOrResourceSelector(ResourceSelectorBuilder* builder);
void FreeResourceSelectorBuilder(ResourceSelectorBuilder* builder);

static inline bool IsResourceSelectorBuilderEmpty(ResourceSelectorBuilder* builder) {
  ASSERT(builder);
  return builder->selectors_len == 0;
}

static inline void AppendResourceSelector(ResourceSelectorBuilder* dst, ResourceSelector* src) {
  ASSERT(dst);
  ASSERT(src);
  return AppendResourceSelectors(dst, &src, 1);
}

static inline void AppendResourceSelectorBuilder(ResourceSelectorBuilder* dst, ResourceSelectorBuilder* src) {
  ASSERT(dst);
  ASSERT(src);
  if (IsResourceSelectorBuilderEmpty(src))
    return;

  return AppendResourceSelectors(dst, src->selectors, src->selectors_len);
}

#endif  // HYPHA_RESOURCE_SELECTOR_H
