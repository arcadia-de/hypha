#include <stdlib.h>

#include "hypha.h"
#include "hypha/query.h"
#include "hypha/resource_query_schema.h"

static const char* ResourceStateName(ResourceState state) {
  switch (state) {
    case kResourcePending:
      return "Pending";
    case kResourceProcessing:
      return "Processing";
    case kResourceReady:
      return "Ready";
    case kResourceFailed:
      return "Failed";
    default:
      return "Unknown";
  }
}

static FieldResolverResult ResourceFieldId(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(((Resource*)obj)->id)};
}

static FieldResolverResult ResourceFieldKind(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(((Resource*)obj)->kind)};
}

static FieldResolverResult ResourceFieldState(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar,
                               .scalar = strdup(ResourceStateName(((Resource*)obj)->state))};
}

static FieldResolverResult ResourceFieldLabels(void* obj) {
  Resource* res = (Resource*)obj;
  const uint32_t n = res->info.labels_len;
  char** list = (char**)malloc(sizeof(char*) * (n > 0 ? n : 1));
  for (uint32_t i = 0; i < n; i++)
    list[i] = strdup(res->info.labels[i]);

  return (FieldResolverResult){.kind = kQueryFieldResultScalarList, .scalar_list = list, .scalar_list_count = n};
}

static FieldResolverResult ResourceFieldDependsOn(void* obj) {
  Resource* res = (Resource*)obj;
  const uint32_t n = res->num_depends_on;
  char** list = (char**)malloc(sizeof(char*) * (n > 0 ? n : 1));
  for (uint32_t i = 0; i < n; i++)
    list[i] = strdup(res->depends_on[i]);

  return (FieldResolverResult){.kind = kQueryFieldResultScalarList, .scalar_list = list, .scalar_list_count = n};
}

static FieldResolverResult ResourceFieldAnnotations(void* obj) {
  Resource* res = (Resource*)obj;
  const uint32_t n = res->info.annotations_len;
  QueryObject* list = (QueryObject*)malloc(sizeof(QueryObject) * (n > 0 ? n : 1));
  for (uint32_t i = 0; i < n; i++)
    list[i] = (QueryObject){.object = &res->info.annotations[i], .type_name = "ResourceAnnotation"};

  return (FieldResolverResult){.kind = kQueryFieldResultObjectList, .object_list = list, .object_list_count = n};
}

static const FieldDef kResourceFields[] = {
#define FIELD(Key, Name) {#Key, ResourceField##Name}

    FIELD(id, Id),
    FIELD(kind, Kind),
    FIELD(state, State),
    FIELD(labels, Labels),
    FIELD(depends_on, DependsOn),
    FIELD(annotations, Annotations),

#undef FIELD
};

static FieldResolverResult AnnotationFieldName(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(((ResourceAnnotation*)obj)->name)};
}

static FieldResolverResult AnnotationFieldValue(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(((ResourceAnnotation*)obj)->value)};
}

static const FieldDef kAnnotationFields[] = {
    {"name", AnnotationFieldName},
    {"value", AnnotationFieldValue},
};

static const TypeDef kTypes[] = {
    {"Resource", kResourceFields, 6},
    {"ResourceAnnotation", kAnnotationFields, 2},
};

static RootResult ResolveResources(const QueryArg* args, void* context) {
  ResourcesQueryContext* ctx = (ResourcesQueryContext*)context;

  const char* kind_filter = QueryArgGet(args, "kind");
  const char* id_filter = QueryArgGet(args, "id");
  const char* label_filter = QueryArgGet(args, "label");

  QueryObject* matched = (QueryObject*)malloc(sizeof(QueryObject) * (ctx->count > 0 ? ctx->count : 1));
  uint32_t n = 0;

  for (uint32_t i = 0; i < ctx->count; i++) {
    Resource* res = &ctx->resources[i];

    if (kind_filter && strcmp(res->kind, kind_filter) != 0)
      continue;

    if (id_filter && strcmp(res->id, id_filter) != 0)
      continue;

    if (label_filter && !ResourceHasLabel(res, label_filter))
      continue;

    matched[n++] = (QueryObject){.object = res, .type_name = "Resource"};
  }

  return (RootResult){.objects = matched, .count = n};
}

static const RootFieldDef kRoots[] = {
    {"resources", ResolveResources, "Resource"},
};

QuerySchema HyphaResourcesQuerySchema(ResourcesQueryContext* ctx) {
  return (QuerySchema){
      .roots = kRoots,
      .num_roots = 1,
      .types = kTypes,
      .num_types = 2,
      .context = ctx,
  };
}
