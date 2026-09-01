#include "hypha/resource_bootstrap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "hypha/log.h"
#include "hypha/resource.h"
#include "hypha/resource_kind.h"
#include "hypha/resource_selector.h"

static const uuid_t kHyphaBootstrapNamespaceUuid = {0xbd, 0x84, 0x2c, 0x9a, 0xef, 0x83, 0x49, 0x52,
                                                    0xa6, 0xa7, 0xda, 0x55, 0xd7, 0x33, 0x0c, 0xd4};

static const AnnotationKey kDocsAnnotationKey = "hypha/docs";
const AnnotationKey kProvidesAnnotationKey = "hypha/provides";

static inline void DeriveBootstrapResourceId(const char* ns, const char* kind, const char* name, ResourceId* out) {
  char key[512];
  snprintf(key, sizeof(key), "%s/%s/%s", ns, kind, name);
  uuid_generate_sha1(*out, kHyphaBootstrapNamespaceUuid, key, strlen(key));
}

static inline bool SetProvidesAnnotation(Resource* res, const char* provides) {
  AnnotationValue value;
  memset(value, 0, sizeof(AnnotationValue));
  strncpy(value, provides, sizeof(AnnotationValue));
  ResourcePushAnnotation(res, &kProvidesAnnotationKey, &value);
  return true;
}

static inline bool SetDocsAnnotation(Resource* res, const char* docs) {
  AnnotationValue value;
  memset(value, 0, sizeof(AnnotationValue));
  strncpy(value, docs, sizeof(AnnotationValue));
  ResourcePushAnnotation(res, &kDocsAnnotationKey, &value);
  return true;
}

bool BootstrapCoreResources(ResourceGraph* graph, const CoreResourceDef* defs, const size_t num_defs) {
  if (!graph || !defs)
    return false;

  for (size_t i = 0; i < num_defs; i++) {
    const CoreResourceDef* def = &defs[i];
    if (!def->kind || !def->name) {
      LOG_ERROR("skipping core resource def %zu: kind and name are required", i);
      continue;
    }

    Resource* res = AllocNewResouceInGraph(graph);
    if (!res) {
      LOG_ERROR("failed to allocate graph slot for core resource %s/%s", def->kind, def->name);
      return false;
    }

    const char* ns = def->ns ? def->ns : kCoreResourceNamespace;

    res->kind = FindOrCreateResourceKind(def->kind);
    if (res->kind == kInvalidResourceKind) {
      LOG_ERROR("failed to find resource kind for: %s", def->kind);
      for (size_t i = 0; i < GetTotalNumberOfResourceKinds(); i++) {
        LOG_ERROR("- %s", GetResourceKindInfo((ResourceKind)i));
      }

      LOG_FATAL("");
    }

    res->info.name = strdup(def->name);
    if (!res->info.name) {
      LOG_ERROR("failed to allocate identity for core resource %s/%s", def->kind, def->name);
      return false;
    }

    SetResourceNamespace(res->info.ns, ns);
    DeriveBootstrapResourceId(ns, def->kind, def->name, &res->id);

    res->flags = def->flags;
    res->state = kResourceReady;
    res->status.state = kResourceReady;
    if (def->provides && !SetProvidesAnnotation(res, def->provides))
      LOG_ERROR("failed to record 'provides' for core resource %s/%s -- continuing without it", def->kind, def->name);

    if (def->docs && !SetDocsAnnotation(res, def->docs))
      LOG_ERROR("failed to record `docs` for core resource %s/%s", def->kind, def->name);
  }

  return true;
}

typedef struct {
  ResourceKind kind;
  const char* provides;
  Resource* found;
} FindProvidesContext;

static inline bool VisitFindProvides(const ResourceGraphIndex idx, Resource* res, void* data) {
  FindProvidesContext* ctx = (FindProvidesContext*)data;
  if (res->kind == kInvalidResourceKind || res->kind != ctx->kind)
    return true;  // keep looking

  Annotation* found = NULL;
  if (!ResourceGetAnnotation(res, &kProvidesAnnotationKey, &found) || !found)
    return true;  // keep looking

  if (strncmp(found->value, ctx->provides, HYPHA_ANNOTATION_VALUE_SIZE) != 0)
    return true;  // keep looking

  ctx->found = res;
  return false;  // stop -- match found
}

bool FindResourceProviding(ResourceGraph* graph, const char* kind, const char* provides, Resource** out) {
  if (!graph || !kind || !provides) {
    if (out)
      *out = NULL;
    return false;
  }

  ResourceSelector* selectors[2];
  selectors[0] = NewKindResourceSelector(kind);
  Annotation annotation;
  memset(&annotation, 0, sizeof(Annotation));
  memcpy(&annotation.key[0], &kProvidesAnnotationKey[0], sizeof(AnnotationKey));
  memcpy(&annotation.value[0], provides, strlen(provides));
  selectors[1] = NewAnnotationResourceSelector(&annotation);
  Resource* found = FindResourceMatching(graph, NewAndResourceSelector(selectors, 2));
  if (out)
    *out = found;
  return true;
}
