#ifndef HYPHA_RESOURCE_ANNOTATION_H
#define HYPHA_RESOURCE_ANNOTATION_H

#include <stdlib.h>
#include <string.h>

typedef struct {
  char* name;
  char* value;
} ResourceAnnotation;

static inline void FreeResourceAnnotation(ResourceAnnotation* rhs) {
  if (!rhs)
    return;
  if (rhs->name)
    free(rhs->name);
  if (rhs->value)
    free(rhs->value);
  free(rhs);
}

static inline ResourceAnnotation* CloneResourceAnnotation(const ResourceAnnotation* rhs) {
  if (!rhs)
    return NULL;
  ResourceAnnotation* clone = (ResourceAnnotation*)malloc(sizeof(ResourceAnnotation));
  if (clone) {
    clone->name = strdup(rhs->name);
    clone->value = strdup(rhs->value);
  }

  return clone;
}

static inline ResourceAnnotation* NewResourceAnnotation(const char* name, const char* value) {
  ResourceAnnotation* annotation = (ResourceAnnotation*)malloc(sizeof(ResourceAnnotation));
  if (annotation) {
    annotation->name = strdup(name);
    annotation->value = strdup(value);
  }

  return annotation;
}

static inline void DeleteResourceAnnotation(ResourceAnnotation* res) {
  if (!res)
    return;

  if (res->name)
    free(res->name);
  if (res->value)
    free(res->value);
  free(res);
}

#endif  // HYPHA_RESOURCE_ANNOTATION_H
