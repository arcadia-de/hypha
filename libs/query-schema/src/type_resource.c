#include "hypha.h"
#include "hypha/assertions.h"
#include "hypha/query.h"
#include "hypha/resource.h"
#include "hypha/resource_kind.h"
#include "hypha/resource_query_schema.h"
#include "hypha/resource_selector.h"
#include "hypha/resource_state.h"
#include "types.h"

#define FIELD_RESOLVER_FN(Name) _FIELD_RESOLVER_FN(Resource, Resource, Name)

FIELD_RESOLVER_FN(id) {
  ResourceIdStr id_str;
  ResourceIdCStr(&ptr->id, id_str);
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(id_str)};
}

FIELD_RESOLVER_FN(kind) {
  const char* name = FindResourceKindName(ptr->kind);
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(name)};
}

FIELD_RESOLVER_FN(state) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(ResourceStateCStr(ptr->state))};
}

FIELD_RESOLVER_FN(labels) {
  const uint32_t n = ptr->info.labels_len;
  char** list = (char**)malloc(sizeof(char*) * (n > 0 ? n : 1));
  for (uint32_t i = 0; i < n; i++)
    list[i] = strdup(ptr->info.labels[i]);

  return (FieldResolverResult){.kind = kQueryFieldResultScalarList, .scalar_list = list, .scalar_list_count = n};
}

FIELD_RESOLVER_FN(depends_on) {
  const uint32_t n = ptr->num_depends_on;
  char** list = (char**)malloc(sizeof(char*) * (n > 0 ? n : 1));
  for (uint32_t i = 0; i < n; i++)
    list[i] = strdup(ptr->depends_on[i]);

  return (FieldResolverResult){.kind = kQueryFieldResultScalarList, .scalar_list = list, .scalar_list_count = n};
}

FIELD_RESOLVER_FN(annotations) {
  const uint32_t n = ptr->info.annotations_len;
  QueryObject* list = (QueryObject*)malloc(sizeof(QueryObject) * (n > 0 ? n : 1));
  for (uint32_t i = 0; i < n; i++)
    list[i] = (QueryObject){.object = &ptr->info.annotations[i], .type_name = "ResourceAnnotation"};

  return (FieldResolverResult){.kind = kQueryFieldResultObjectList, .object_list = list, .object_list_count = n};
}

FIELD_RESOLVER_FN(name) {
  return (FieldResolverResult){
      .kind = kQueryFieldResultScalar,
      .scalar = strdup(ptr->info.name),
  };
}

#define FOR_EACH_RESOURCE_FIELD(V) \
  V(id)                            \
  V(kind)                          \
  V(name)                          \
  V(state)                         \
  V(labels)                        \
  V(depends_on)                    \
  V(annotations)

static inline ResourceSelector* CreateSelector(ResourcesQueryContext* ctx, const QueryArg* args) {
  ASSERT(ctx);

  ResourceSelectorBuilder builder;
  InitResourceSelectorBuilder(&builder, 10);
  if (args) {
#define DEFINE_FILTER(Field, Filter)                \
  ({                                                \
    const char* filter = QueryArgGet(args, #Field); \
    if (filter)                                     \
      AppendResourceSelector(&builder, (Filter));   \
  })

    DEFINE_FILTER(id, NewRefResourceSelector(filter));
    DEFINE_FILTER(kind, NewKindResourceSelector(filter));
#undef DEFINE_FILTER
  }

  if (IsResourceSelectorBuilderEmpty(&builder)) {
    FreeResourceSelectorBuilder(&builder);
    return NULL;
  }

  return BuildAndResourceSelector(&builder);
}

DEFINE_ROOT_TYPE(Resource, resources, FOR_EACH_RESOURCE_FIELD, 7) {
  ResourcesQueryContext* ctx = (ResourcesQueryContext*)data;
  ASSERT(ctx);
  ResourceSelector* selector = CreateSelector(ctx, args);

  size_t n = 0;
  QueryObject* matched = (QueryObject*)calloc(sizeof(QueryObject), ctx->count);
  if (matched) {
    for (size_t i = 0; i < ctx->count; i++) {
      Resource* res = &ctx->resources[i];
      ASSERT(res);
      if (selector && !ResourceSelectorMatch(selector, res))
        continue;
      matched[n++] = (QueryObject){.object = res, .type_name = "Resource"};
    }
  }

  return (RootResult){.objects = matched, .count = n};
}
