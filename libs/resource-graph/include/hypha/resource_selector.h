#ifndef HYPHA_RESOURCE_SELECTOR_H
#define HYPHA_RESOURCE_SELECTOR_H

#include "hypha/resource.h"

typedef bool (*ResourceSelectorFn)(const Resource*, void* data);

typedef struct _ResourceSelector ResourceSelector;

ResourceSelector* NewResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*));
ResourceSelector* NewAndResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
ResourceSelector* NewOrResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
ResourceSelector* NewRefResourceSelector(const char* rhs);
ResourceSelector* NewKindResourceSelector(const char* rhs);
ResourceSelector* NewLabelResourceSelector(const Label* rhs);
ResourceSelector* NewNamespaceResourceSelector(const char* ns);
ResourceSelector* NewAnnotationResourceSelector(const Annotation* rhs);
ResourceSelector* NewAnnotationKeyResourceSelector(const AnnotationKey* rhs);
ResourceSelector* NewAnnotationValueResourceSelector(const AnnotationValue* rhs);
bool ResourceSelectorMatch(const ResourceSelector* rs, const Resource* res);
void FreeResourceSelector(ResourceSelector* rs);

#endif  // HYPHA_RESOURCE_SELECTOR_H
