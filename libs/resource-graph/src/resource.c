#include "hypha/resource.h"

#include <string.h>
#include <xxhash.h>

#include "hypha.h"
#include "hypha/assertions.h"
#include "hypha/label.h"
#include "hypha/log.h"

void FreeResourceSpecJson(ResourceSpecDocument* rhs) {
  if (!rhs)
    return;
  if (rhs->doc)
    json_decref(rhs->doc);
  rhs->doc = NULL;
}

bool ResourceSpecParseJson(ResourceSpecDocument* doc) {
  DLOG_FATAL_IF(doc == NULL || doc->raw == NULL, "doc is null");

  json_error_t err;
  json_t* json_doc = json_loads(doc->raw, 0, &err);
  if (!json_doc) {
    DLOG_ERROR("invalid spec doc:\n%s", doc->raw);
    DLOG_ERROR("error on line %d: %s", err.line, err.text);
    return false;
  }

  doc->doc = json_doc;
  return true;
}

bool ResourceHasId(const Resource* res, const char* id) {
  if (!res || !id)
    return false;

  ResourceIdStr str;
  ResourceIdCStr(&res->id, str);
  return strcmp(str, id) == 0;
}

bool ResourceMatchesRef(const Resource* res, const char* ref) {
  if (!res || !ref)
    return false;

  if (res->info.name && strcmp(res->info.name, ref) == 0)
    return true;

  return ResourceHasId(res, ref);
}

bool ResourceHasLabel(const Resource* res, const Label rhs) {
  if (!res || !rhs)
    return false;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->labels_len; i++) {
    if (LabelEq(info->labels[i], rhs))
      return true;
  }

  return false;
}

void ResourceVisitLabels(const Resource* res, VisitResourceLabelFn fn, void* data) {
  ASSERT(res);
  ASSERT(fn);

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->labels_len; i++) {
    if (!fn(i, info->labels[i], data))
      return;
  }
}

bool ResourceVisitAnnotations(const Resource* res, VisitResourceAnnotationFn fn, void* data) {
  bool result = false;
  if (!res)
    goto success;

  const ResourceInfo* info = &res->info;
  for (size_t i = 0; i < info->annotations_len; i++) {
    if (!fn(i, &info->annotations[i], data))
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

bool ResourceVisitDependsOn(const Resource* res, VisitResourceDependencyFn fn, void* data) {
  bool result = false;
  if (!res || !fn)
    goto finished;

  for (size_t i = 0; i < res->num_depends_on; i++) {
    if (!fn(i, res->depends_on[i], data))
      goto finished;
  }

  result = true;
finished:
  return result;
}

static inline void EnsureLabelsCapacity(ResourceInfo* info, const size_t num_labels) {
  if (num_labels < info->labels_cap)
    return;

  const size_t new_cap = (info->labels_cap + num_labels + 1);  // TODO(@s0cks): round up pow2
  const size_t total_size = sizeof(Label) * new_cap;
  Label* new_labels = (Label*)realloc(info->labels, total_size);
  LOG_FATAL_IF(!new_labels, "failed to allocate new %zu new resource labels", new_cap);
  info->labels = new_labels;
  info->labels_cap = new_cap;
}

void ResourcePushLabels(Resource* res, const Label* labels, const size_t num_labels) {
  ASSERT(res);
  ASSERT(labels);
  ASSERT(num_labels > 0);
  ResourceInfo* info = &res->info;
  EnsureLabelsCapacity(info, info->labels_len + num_labels);

  const size_t total_size = sizeof(Label) * num_labels;
  memcpy(&info->labels[info->labels_len], &labels[0], total_size);
  info->labels_len += num_labels;
}

void ResourcePushLabel(Resource* res, const Label label) {
  ASSERT(res);
  ASSERT(label);
  ResourceInfo* info = &res->info;
  EnsureLabelsCapacity(info, info->labels_len + 1);
  memcpy(&info->labels[info->labels_len], label, HYPHA_LABEL_MAX_SIZE);
  info->labels_len++;
}

void ResourcePushAnnotation(Resource* res, const AnnotationKey* k, const AnnotationValue* v) {
  ASSERT(res);
  ASSERT(k);
  ASSERT(v);
  ResourceInfo* info = &res->info;
  if ((info->annotations_len + 1) >= info->annotations_cap) {
    const size_t new_cap = (info->annotations_cap + 1) * 2;
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

  if (info->name)
    free(info->name);

  if (info->annotations)
    free(info->annotations);

  if (info->labels)
    free(info->labels);
}
