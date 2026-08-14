#ifndef HYPHA_RESOURCE_H
#define HYPHA_RESOURCE_H

#include <stdint.h>
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

typedef struct {
  // labels
  char** labels;
  uint64_t labels_len;
  uint64_t labels_cap;
  // annotations
  ResourceAnnotation* annotations;
  uint64_t annotations_len;
  uint64_t annotations_cap;
} ResourceInfo;

void FreeResourceInfo(ResourceInfo* info);

#define FOR_EACH_RESOURCE_STATE(V) \
  V(Pending)                       \
  V(Processing)                    \
  V(Ready)                         \
  V(Failed)                        \
  V(Unknown)

// clang-format off
typedef enum {
#define DEFINE_STATE(Name) kResource##Name,
  FOR_EACH_RESOURCE_STATE(DEFINE_STATE)
#undef DEFINE_STATE
  kTotalNumberOfResourceStates,
} ResourceState;
// clang-format on

typedef struct {
  char* id;
  char* kind;
  ResourceInfo info;
  char** depends_on;
  uint32_t num_depends_on;
  ResourceState state;
  char* spec;
} Resource;

#define DEFINE_STATE_CHECK(Name)                             \
  static inline bool IsResource##Name(const Resource* res) { \
    return res && res->state == kResource##Name;             \
  }
FOR_EACH_RESOURCE_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

bool ResourceHasLabel(const Resource* res, const char* label);
bool ResourceHasAnnotation(const Resource* res, const ResourceAnnotation* annotation);
bool ResourceHasAnnotationK(const Resource* res, const char* label);
bool ResourceHasAnnotationV(const Resource* res, const char* value);
bool ResourceHasAnnotationKV(const Resource* res, const char* label, const char* value);
bool ResourceVisitLabels(const Resource* res, bool (*vis)(const Resource*, const uint32_t, const char*));
bool ResourceVisitAnnotations(const Resource* res,
                              bool (*vis)(const Resource*, const uint32_t, const ResourceAnnotation*));
bool ResourceVisitDependsOn(const Resource* res, bool (*vis)(const Resource*, const uint32_t, const char*));
void ResourcePushLabel(Resource* res, const char* label);
void ResourcePushAnnotation(Resource* res, const char* k, const char* v);

#define BEGIN_FOREACH_RESOURCE_DEPENDSON(Resource, Name)    \
  for (uint32_t i = 0; i < Resource->num_depends_on; i++) { \
    const char* Name = Resource->depends_on[i];

#define END_FOREACH_RESOURCE_DEPENDSON }

#define BEGIN_FOREACH_RESOURCE_LABEL(Resource, Name)           \
  const ResourceInfo* Resource##_info = &(Resource)->info;     \
  for (uint32_t i = 0; i < Resource##_info->labels_len; i++) { \
    const char* Name = Resource##_info->labels[i];

#define END_FOREACH_RESOURCE_LABEL }

#define BEGIN_FOREACH_RESOURCE_ANNOTATION(Resource, Name)           \
  const ResourceInfo* Resource##_info = &(Resource)->info;          \
  for (uint32_t i = 0; i < Resource##_info->annotations_len; i++) { \
    const ResourceAnnotation* Name = &Resource##_info->annotations[i];

#define END_FOREACH_RESOURCE_ANNOTATION }

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

static inline void PushResourceAnnotation(Resource* res, const ResourceAnnotation* rhs) {
  ResourceInfo* info = &res->info;
  memcpy(&info->annotations[info->annotations_len], rhs, sizeof(ResourceAnnotation));
  info->annotations_len++;
}

#endif  // HYPHA_RESOURCE_H
