#include "hypha/resource.h"

#include <string.h>

#include "hypha.h"
#include "hypha/log.h"

bool ResourceHasId(const Resource* res, const char* id) {
  if (!res || !id)
    return false;
  return strcmp(res->id, id) == 0;
}

bool ResourceHasLabel(const Resource* res, const char* label) {
  if (!res || !label)
    return false;

  BEGIN_FOREACH_RESOURCE_LABEL(res, lhs)
  if (strcmp(lhs, label) == 0)
    return true;
  END_FOREACH_RESOURCE_LABEL

  return false;
}

bool ResourceVisitLabels(const Resource* res, bool (*vis)(const Resource*, const uint32_t, const char*)) {
  bool result = false;
  if (!res)
    goto success;

  BEGIN_FOREACH_RESOURCE_LABEL(res, label);
  if (!vis(res, i, label))
    goto finished;
  END_FOREACH_RESOURCE_LABEL

success:
  result = true;
finished:
  return result;
}

bool ResourceVisitAnnotations(const Resource* res,
                              bool (*vis)(const Resource*, const uint32_t, const ResourceAnnotation*)) {
  bool result = false;
  if (!res)
    goto success;

  BEGIN_FOREACH_RESOURCE_ANNOTATION(res, annot)
  if (!vis(res, i, annot))
    goto finished;
  END_FOREACH_RESOURCE_ANNOTATION

success:
  result = true;
finished:
  return result;
}

static inline bool ResourceAnnotationEq(const ResourceAnnotation* lhs, const ResourceAnnotation* rhs) {
  if (!lhs && !rhs)
    return true;

  if ((!lhs && rhs) || (lhs && !rhs))
    return false;

  return strcmp(lhs->name, rhs->name) == 0 && strcmp(lhs->value, rhs->value) == 0;
}

bool ResourceGetAnnotation(const Resource* res, const char* name, const char** result) {
  if (!res || !name)
    goto finished;

  BEGIN_FOREACH_RESOURCE_ANNOTATION(res, lhs)
  if (strcmp(lhs->name, name) == 0) {
    (*result) = lhs->value;
    return true;
  }
  END_FOREACH_RESOURCE_ANNOTATION;

finished:
  (*result) = NULL;
  return false;
}

bool ResourceHasAnnotation(const Resource* res, const ResourceAnnotation* rhs) {
  if (!res || !rhs)
    return false;

  BEGIN_FOREACH_RESOURCE_ANNOTATION(res, lhs)
  if (ResourceAnnotationEq(lhs, rhs))
    return true;
  END_FOREACH_RESOURCE_ANNOTATION

  return false;
}

bool ResourceHasAnnotationK(const Resource* res, const char* label) {
  if (!res || !label)
    return false;

  BEGIN_FOREACH_RESOURCE_ANNOTATION(res, lhs)
  if (strcmp(lhs->name, label) == 0)
    return true;
  END_FOREACH_RESOURCE_ANNOTATION

  return false;
}

bool ResourceHasAnnotationV(const Resource* res, const char* value) {
  if (!res || !value)
    return false;

  BEGIN_FOREACH_RESOURCE_ANNOTATION(res, lhs)
  if (strcmp(lhs->value, value) == 0)
    return true;
  END_FOREACH_RESOURCE_ANNOTATION;

  return false;
}

bool ResourceHasAnnotationKV(const Resource* res, const char* label, const char* value) {
  if (!res || !label || !value)
    return false;

  const ResourceAnnotation annotation = {
      .name = (char*)label,
      .value = (char*)value,
  };
  BEGIN_FOREACH_RESOURCE_ANNOTATION(res, lhs)
  if (ResourceAnnotationEq(lhs, &annotation))
    return true;
  END_FOREACH_RESOURCE_ANNOTATION

  return false;
}

bool ResourceVisitDependsOn(const Resource* res, bool (*vis)(const Resource*, const uint32_t, const char*)) {
  bool result = false;
  if (!res || !vis)
    goto finished;

  BEGIN_FOREACH_RESOURCE_DEPENDSON(res, depends)
  if (!vis(res, i, depends))
    goto finished;
  END_FOREACH_RESOURCE_DEPENDSON

  result = true;
finished:
  return result;
}

void ResourcePushLabel(Resource* res, const char* label) {
  ASSERT(res);
  ASSERT(label);
  ResourceInfo* info = &res->info;
  if ((info->labels_len + 1) >= info->labels_cap) {
    const uint64_t new_cap = info->labels_cap * 2;
    char** new_labels = (char**)realloc(info->labels, new_cap);
    if (!new_labels) {
      LOG_FATAL("failed to allocate new resource labels of size: %lu", new_cap);
      return;
    }

    info->labels = new_labels;
    info->labels_cap = new_cap;
  }
  info->labels[info->labels_len] = strdup(label);
  info->labels_len++;
}

void ResourcePushAnnotation(Resource* res, const char* k, const char* v) {
  ASSERT(res);
  ASSERT(k);
  ASSERT(v);
  ResourceInfo* info = &res->info;
  if ((info->annotations_len + 1) >= info->annotations_cap) {
    const uint64_t new_cap = info->annotations_cap * 2;
    ResourceAnnotation* new_annotations = (ResourceAnnotation*)realloc(info->annotations, new_cap);
    if (!new_annotations) {
      LOG_FATAL("failed to allocate new annotations of size: %lu", new_cap);
      return;
    }

    info->annotations = new_annotations;
    info->annotations_cap = new_cap;
  }

  ResourceAnnotation new_annotation = {
      .name = strdup(k),
      .value = strdup(v),
  };
  memmove(&info->annotations[info->annotations_len], &new_annotation, sizeof(ResourceAnnotation));
  info->annotations_len++;
}

void FreeResourceInfo(ResourceInfo* info) {
  if (!info)
    return;

  if (info->annotations)
    free(info->annotations);

  if (info->labels)
    free(info->labels);
}
