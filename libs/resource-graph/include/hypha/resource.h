#ifndef HYPHA_RESOURCE_H
#define HYPHA_RESOURCE_H

#include <jansson.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hypha.h"
#include "hypha/annotation.h"
#include "hypha/label.h"
#include "hypha/resource_id.h"
#include "hypha/resource_state.h"

typedef struct {
  char* name;

  // labels
  Label* labels;
  size_t labels_len;
  size_t labels_cap;

  // annotations
  Annotation* annotations;
  size_t annotations_len;
  size_t annotations_cap;
} ResourceInfo;

void FreeResourceInfo(ResourceInfo* info);

typedef struct {
  char* raw;
  json_t* doc;
  uint64_t hash;
} ResourceSpecDocument;

uint64_t ResourceSpecDocumentGetHash(ResourceSpecDocument* rhs);

typedef struct {
  struct {
    struct timespec start;
    struct timespec finish;
  } states[kTotalNumberOfOrchestratorStates];
} ResourceTelemetry;

typedef struct {
  ResourceState state;
  ControllerAction action;
  struct timespec timestamp;
  Reason reason;
} ResourceStatus;

struct _Resource {
  ResourceId id;
  char* kind;
  ResourceInfo info;
  ResourceState state;  // TODO(@s0cks): replace w/ status field usage
  ResourceStatus status;
  ResourceSpecDocument spec;

  char** depends_on;
  size_t num_depends_on;
  size_t depends_on_cap;

  ResourceTelemetry telemetry;
};

#define DEFINE_STATE_CHECK(Name)                             \
  static inline bool IsResource##Name(const Resource* res) { \
    return res && res->state == kResource##Name;             \
  }
FOR_EACH_RESOURCE_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

bool ResourceHasId(const Resource* res, const char* id);

// Matches a resource against a user-supplied reference that may be either its
// name or its id. Name is checked first since that's what a person is expected
// to type; id (uuid) is the fallback for machine/controller-supplied references.
bool ResourceMatchesRef(const Resource* res, const char* ref);
void ResourcePushLabel(Resource* res, const Label* label);
bool ResourceHasLabel(const Resource* res, const Label* label);
bool ResourceVisitLabels(const Resource* res, bool (*vis)(const Resource*, const uint64_t, const Label*));

static inline void PushResourceAnnotation(Resource* res, const Annotation* rhs) {
  ResourceInfo* info = &res->info;
  memcpy(&info->annotations[info->annotations_len], rhs, sizeof(Annotation));
  info->annotations_len++;
}

void ResourcePushAnnotation(Resource* res, const AnnotationKey* k, const AnnotationValue* v);
bool ResourceGetAnnotation(const Resource* res, const AnnotationKey* k, Annotation** result);
bool ResourceHasAnnotation(const Resource* res, const Annotation* annotation);
bool ResourceHasAnnotationK(const Resource* res, const AnnotationKey* k);
bool ResourceHasAnnotationV(const Resource* res, const AnnotationValue* v);
bool ResourceHasAnnotationKV(const Resource* res, const AnnotationKey* k, const AnnotationValue* v);
bool ResourceVisitAnnotations(const Resource* res, bool (*vis)(const Resource*, const uint64_t, const Annotation*));

bool ResourceVisitDependsOn(const Resource* res, bool (*vis)(const Resource*, const uint64_t, const char*));

#endif  // HYPHA_RESOURCE_H
