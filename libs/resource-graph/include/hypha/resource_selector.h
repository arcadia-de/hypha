#ifndef HYPHA_RESOURCE_SELECTOR_H
#define HYPHA_RESOURCE_SELECTOR_H

#include "hypha/resource.h"

typedef bool (*ResourceSelectorFn)(const Resource*, void* data);

typedef struct _ResourceSelector ResourceSelector;

ResourceSelector* NewResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*));
ResourceSelector* NewAndResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
ResourceSelector* NewOrResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
ResourceSelector* NewKindResourceSelector(const char* rhs);
ResourceSelector* NewLabelResourceSelector(const char* rhs);
ResourceSelector* NewAnnotationResourceSelector(const ResourceAnnotation* rhs);
ResourceSelector* NewAnnotationKeyResourceSelector(const char* rhs);
ResourceSelector* NewAnnotationValueResourceSelector(const char* rhs);
bool ResourceSelectorMatch(const ResourceSelector* rs, const Resource* res);
void FreeResourceSelector(ResourceSelector* rs);

#endif  // HYPHA_RESOURCE_SELECTOR_H
