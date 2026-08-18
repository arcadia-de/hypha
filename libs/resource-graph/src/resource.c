#include "hypha/resource.h"

#include <string.h>
#include <xxhash.h>

#include "hypha.h"
#include "hypha/label.h"
#include "hypha/log.h"

bool ResourceHasId(const Resource* res, const char* id) {
  if (!res || !id)
    return false;
  return strcmp(res->id, id) == 0;
}

bool ResourceHasLabel(const Resource* res, const Label* rhs) {
  if (!res || !rhs)
    return false;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->labels_len; i++) {
    if (LabelEq(&info->labels[i], rhs))
      return true;
  }

  return false;
}

bool ResourceVisitLabels(const Resource* res, bool (*vis)(const Resource*, const uint64_t, const Label*)) {
  bool result = false;
  if (!res)
    goto success;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->labels_len; i++) {
    if (!vis(res, i, &info->labels[i]))
      goto finished;
  }

success:
  result = true;
finished:
  return result;
}

bool ResourceVisitAnnotations(const Resource* res, bool (*vis)(const Resource*, const uint64_t, const Annotation*)) {
  bool result = false;
  if (!res)
    goto success;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->annotations_len; i++) {
    if (!vis(res, i, &info->annotations[i]))
      goto finished;
  }

success:
  result = true;
finished:
  return result;
}

bool ResourceGetAnnotation(const Resource* res, const AnnotationKey* k, Annotation** result) {
  if (!res || !k)
    goto finished;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->annotations_len; i++) {
    Annotation* lhs = &info->annotations[i];
    if (AnnotationKeyEq(&lhs->key, k)) {
      (*result) = lhs;
      return true;
    }
  }

finished:
  (*result) = NULL;
  return false;
}

bool ResourceHasAnnotation(const Resource* res, const Annotation* rhs) {
  if (!res || !rhs)
    return false;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->annotations_len; i++) {
    if (AnnotationEq(&info->annotations[i], rhs))
      return true;
  }

  return false;
}

bool ResourceHasAnnotationK(const Resource* res, const AnnotationKey* k) {
  if (!res || !k)
    return false;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->annotations_len; i++) {
    if (AnnotationKeyEq(&info->annotations[i].key, k))
      return true;
  }

  return false;
}

bool ResourceHasAnnotationV(const Resource* res, const AnnotationValue* value) {
  if (!res || !value)
    return false;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->annotations_len; i++) {
    const Annotation* lhs = &info->annotations[i];
    if (AnnotationValueEq(&lhs->value, value))
      return true;
  }

  return false;
}

bool ResourceVisitDependsOn(const Resource* res, bool (*vis)(const Resource*, const uint64_t, const char*)) {
  bool result = false;
  if (!res || !vis)
    goto finished;

  for (size_t i = 0; i < res->num_depends_on; i++) {
    if (!vis(res, i, res->depends_on[i]))
      goto finished;
  }

  result = true;
finished:
  return result;
}

void ResourcePushLabel(Resource* res, const Label* label) {
  ASSERT(res);
  ASSERT(label);
  ResourceInfo* info = &res->info;
  if ((info->labels_len + 1) >= info->labels_cap) {
    const size_t new_cap = info->labels_cap * 2;
    const size_t total_size = sizeof(Label) * new_cap;
    Label* new_labels = (Label*)realloc(info->labels, total_size);
    LOG_FATAL_IF(!new_labels, "failed to allocate new %zu new resource labels", new_cap);
    info->labels = new_labels;
    info->labels_cap = new_cap;
  }

  memcpy(&info->labels[info->labels_len], *label, HYPHA_LABEL_MAX_SIZE);
  info->labels_len++;
}

void ResourcePushAnnotation(Resource* res, const AnnotationKey* k, const AnnotationValue* v) {
  ASSERT(res);
  ASSERT(k);
  ASSERT(v);
  ResourceInfo* info = &res->info;
  if ((info->annotations_len + 1) >= info->annotations_cap) {
    const size_t new_cap = info->annotations_cap * 2;
    const size_t total_size = sizeof(Annotation) * new_cap;
    Annotation* new_annotations = (Annotation*)realloc(info->annotations, total_size);
    LOG_FATAL_IF(!new_annotations, "failed to allocate new annotations of size: %zu", new_cap);

    info->annotations = new_annotations;
    info->annotations_cap = new_cap;
  }

  Annotation new_annotation;
  memcpy(&new_annotation.key, k, HYPHA_ANNOTATION_KEY_SIZE);
  memcpy(&new_annotation.value, v, HYPHA_ANNOTATION_VALUE_SIZE);
  memmove(&info->annotations[info->annotations_len], &new_annotation, sizeof(Annotation));
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
