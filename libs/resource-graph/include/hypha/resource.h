#ifndef HYPHA_RESOURCE_H
#define HYPHA_RESOURCE_H

#include <jansson.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/resource_annotation.h"
#include "hypha/resource_state.h"

typedef struct {
  // labels
  char** labels;
  size_t labels_len;
  size_t labels_cap;
  // annotations
  ResourceAnnotation* annotations;
  size_t annotations_len;
  size_t annotations_cap;
} ResourceInfo;

void FreeResourceInfo(ResourceInfo* info);

typedef struct {
  char* raw;
  json_t* doc;
} ResourceSpecDocument;

// TODO(@s0cks): use a uuid for the id instead of a char*
//  #include <uuid/uuid.h>
//  uuid_t buuid;
//  char str[37];
//  uuid_generate_random(buuid);
//  uuid_unparse(buuid, str);
struct _Resource {
  char* id;
  char* kind;
  ResourceInfo info;
  char** depends_on;
  uint32_t num_depends_on;
  ResourceState state;
  ResourceSpecDocument spec;
};

#define DEFINE_STATE_CHECK(Name)                             \
  static inline bool IsResource##Name(const Resource* res) { \
    return res && res->state == kResource##Name;             \
  }
FOR_EACH_RESOURCE_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

void ResourcePushLabel(Resource* res, const char* label);
bool ResourceHasLabel(const Resource* res, const char* label);
bool ResourceVisitLabels(const Resource* res, bool (*vis)(const Resource*, const uint32_t, const char*));

static inline void PushResourceAnnotation(Resource* res, const ResourceAnnotation* rhs) {
  ResourceInfo* info = &res->info;
  memcpy(&info->annotations[info->annotations_len], rhs, sizeof(ResourceAnnotation));
  info->annotations_len++;
}

void ResourcePushAnnotation(Resource* res, const char* k, const char* v);
bool ResourceGetAnnotation(const Resource* res, const char* name, const char** result);
bool ResourceHasAnnotation(const Resource* res, const ResourceAnnotation* annotation);
bool ResourceHasAnnotationK(const Resource* res, const char* label);
bool ResourceHasAnnotationV(const Resource* res, const char* value);
bool ResourceHasAnnotationKV(const Resource* res, const char* label, const char* value);
bool ResourceVisitAnnotations(const Resource* res,
                              bool (*vis)(const Resource*, const uint32_t, const ResourceAnnotation*));

bool ResourceVisitDependsOn(const Resource* res, bool (*vis)(const Resource*, const uint32_t, const char*));

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

#endif  // HYPHA_RESOURCE_H
