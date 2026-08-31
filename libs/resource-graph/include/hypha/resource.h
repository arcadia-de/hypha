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
#include "hypha/orchestrator_state.h"
#include "hypha/priority.h"
#include "hypha/reason.h"
#include "hypha/resource_flags.h"
#include "hypha/resource_id.h"
#include "hypha/resource_kind.h"
#include "hypha/resource_namespace.h"
#include "hypha/resource_state.h"

typedef struct {
  char* name;
  ResourceNamespace ns;

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

bool ResourceSpecParseJson(ResourceSpecDocument* rhs);
void FreeResourceSpecJson(ResourceSpecDocument* rhs);
uint64_t ResourceSpecDocumentGetHash(ResourceSpecDocument* rhs);

// Guarantees `rhs->doc` is a valid (non-NULL) json object by the time this returns --
// controllers should never have to NULL-check a resource's spec.doc before calling
// json_object_get on it. Covers three cases: `.doc` already set (no-op), `.raw` set but
// unparsed or invalid (parses it, or falls back to an empty object on failure), and
// `.raw` itself NULL (e.g. a Resource reconstructed from a state store lookup that found
// nothing -- happens for both `desired` and `observed` on a resource's first-ever
// reconcile) -- in which case both `.raw` and `.doc` are set to a fresh empty object.
void EnsureResourceSpecDoc(ResourceSpecDocument* rhs);

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
  ResourceKind kind;
  ResourceInfo info;
  ResourceState state;  // TODO(@s0cks): replace w/ status field usage
  ResourceStatus status;
  ResourceSpecDocument spec;

  char** depends_on;
  size_t num_depends_on;
  size_t depends_on_cap;

  Priority priority;
  ResourceFlags flags;
  ResourceTelemetry telemetry;
};

#define DEFINE_STATE_CHECK(Name)                             \
  static inline bool IsResource##Name(const Resource* res) { \
    return res && res->state == kResource##Name;             \
  }
FOR_EACH_RESOURCE_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

static inline bool IsResourceStatic(const Resource* res) {
  return res && ResourceFlagsHas(res->flags, kResourceFlagStatic);
}

static inline bool IsResourceDynamic(const Resource* res) {
  return !IsResourceStatic(res);
}

static inline bool IsResourceSynthetic(const Resource* res) {
  return res && ResourceFlagsHas(res->flags, kResourceFlagSynthetic);
}

typedef bool (*VisitResourceLabelFn)(uint64_t, const Label, void*);
void ResourceVisitLabels(const Resource* res, VisitResourceLabelFn fn, void* data);

typedef bool (*VisitResourceAnnotationFn)(uint64_t, const Annotation*, void*);
bool ResourceVisitAnnotations(const Resource* res, VisitResourceAnnotationFn fn, void* data);

typedef bool (*VisitResourceDependencyFn)(uint64_t, const char*, void* data);
bool ResourceVisitDependsOn(const Resource* res, VisitResourceDependencyFn fn, void* data);

// TODO(@s0cks): cleanup this section
bool ResourceHasId(const Resource* res, const char* id);
bool ResourceMatchesRef(const Resource* res, const char* ref);
void ResourcePushLabel(Resource* res, const Label label);
void ResourcePushLabels(Resource* res, const Label* labels, const size_t num_labels);
bool ResourceHasLabel(const Resource* res, const Label label);

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

#endif  // HYPHA_RESOURCE_H
