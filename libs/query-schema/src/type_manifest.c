#include "hypha/assertions.h"
#include "hypha/query.h"
#include "hypha/resource.h"
#include "hypha/resource_kind.h"
#include "hypha/resource_query_schema.h"
#include "hypha/resource_selector.h"
#include "hypha/resource_state.h"
#include "types.h"

#define FIELD_RESOLVER_FN(Name) _FIELD_RESOLVER_FN(Manifest, Resource, Name)

FIELD_RESOLVER_FN(id) {
  char* id = (char*)malloc(UUID_STR_LEN);
  uuid_unparse_lower(ptr->id, id);
  return (FieldResolverResult){
      .kind = kQueryFieldResultScalar,
      .scalar = id,
  };
}

FIELD_RESOLVER_FN(name) {
  return (FieldResolverResult){
      .kind = kQueryFieldResultScalar,
      .scalar = strdup(ptr->info.name),
  };
}

FIELD_RESOLVER_FN(hash) {
  return (FieldResolverResult){
      .kind = kQueryFieldResultScalar,
      .scalar = NULL,  // TODO(@s0cks): hash to hex
  };
}

FIELD_RESOLVER_FN(namespace) {
  return (FieldResolverResult){
      .kind = kQueryFieldResultScalar,
      .scalar = strdup(ptr->info.ns),
  };
}

FIELD_RESOLVER_FN(labels) {
  const size_t n = ptr->info.labels_len;
  char** list = (char**)malloc(sizeof(char*) * (n > 0 ? n : 1));
  for (size_t i = 0; i < n; i++)
    list[i] = strdup(ptr->info.labels[i]);
  return (FieldResolverResult){.kind = kQueryFieldResultScalarList, .scalar_list = list, .scalar_list_count = n};
}

#define FOR_EACH_MANIFEST_FIELD(V) \
  V(id)                            \
  V(name)                          \
  V(hash)                          \
  V(namespace)                     \
  V(labels)

typedef struct {
  QueryObject* matched;
  size_t matched_len;
} ManifestResolveContext;

static inline ResourceSelector* CreateSelector(ResourcesQueryContext* ctx, const QueryArg* args) {
  ASSERT(ctx);
  // const char* hash_filter = QueryArgGet(args, "hash");
  // const char* path_filter = QueryArgGet(args, "path");
  // const char* source_filter = QueryArgGet(args, "source");
  size_t num_selectors = 0;
  ResourceSelector* selectors[6];
  selectors[num_selectors] = NewKindResourceSelector("Manifest");
  num_selectors++;

  if (args) {
    const char* id_filter = QueryArgGet(args, "id");
    if (id_filter) {
      selectors[num_selectors] = NewRefResourceSelector(id_filter);
      num_selectors++;
    }

    const char* state_filter = QueryArgGet(args, "state");
    if (state_filter) {
      selectors[num_selectors] = NewStateResourceSelector(ParseResourceState(state_filter));
      num_selectors++;
    }

    const char* namespace_filter = QueryArgGet(args, "namespace");
    if (namespace_filter) {
      selectors[num_selectors] = NewNamespaceResourceSelector(namespace_filter);
      num_selectors++;
    }

    const char* name_filter = QueryArgGet(args, "name");
    if (name_filter) {
      selectors[num_selectors] = NewNameResourceSelector(name_filter);
      num_selectors++;
    }
  }

  return NewAndResourceSelector(selectors, num_selectors);
}

DEFINE_ROOT_TYPE(Manifest, manifests, FOR_EACH_MANIFEST_FIELD, 5) {
  ResourcesQueryContext* ctx = (ResourcesQueryContext*)data;
  ASSERT(ctx);
  QueryObject* matched = (QueryObject*)malloc(sizeof(QueryObject) * (ctx->count > 0 ? ctx->count : 1));
  memset(matched, 0, sizeof(QueryObject));
  ResourceSelector* selector = CreateSelector(ctx, args);
  ASSERT(selector);

  size_t n = 0;
  for (size_t i = 0; i < ctx->count; i++) {
    Resource* res = &ctx->resources[i];
    ASSERT(res);
    if (!ResourceSelectorMatch(selector, res))
      continue;
    matched[n++] = (QueryObject){.object = res, .type_name = "Manifest"};
  }

  if (selector)
    FreeResourceSelector(selector);
  return (RootResult){.objects = matched, .count = n};
}
