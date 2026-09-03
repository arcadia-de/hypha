#include "hypha/resource_selector.h"

#include <stdlib.h>

#include "hypha.h"
#include "hypha/annotation.h"
#include "hypha/assertions.h"
#include "hypha/log.h"
#include "hypha/resource.h"

#define FOR_EACH_RESOURCE_SELECTOR_KIND(V) \
  V(Atomic)                                \
  V(Not)                                   \
  V(Negate)                                \
  V(And)                                   \
  V(Or)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) k##Name##Selector,
  FOR_EACH_RESOURCE_SELECTOR_KIND(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfResourceSelectorKinds,
} ResourceSelectorKind;
// clang-format on

struct _ResourceSelector {
  ResourceSelectorKind kind;
  union {
    struct {
      ResourceSelectorFn fn;
      void* data;
      void (*free_data)(void*);
    };

    struct {
      ResourceSelector** selectors;
      uint64_t num_selectors;
    };
  };
};

#define DEFINE_CHECK(Name)                                      \
  static inline bool Is##Name##Selector(ResourceSelector* rs) { \
    return rs && rs->kind == k##Name##Selector;                 \
  }
FOR_EACH_RESOURCE_SELECTOR_KIND(DEFINE_CHECK)
#undef DEFINE_CHECK

static inline bool IsCompositeSelector(ResourceSelector* rs) {
  return IsAndSelector(rs) || IsOrSelector(rs);
}

ResourceSelector* NewResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*)) {
  ASSERT(fn);
  ResourceSelector* selector = (ResourceSelector*)malloc(sizeof(ResourceSelector));
  if (selector) {
    selector->kind = kAtomicSelector;
    selector->fn = fn;
    selector->data = data;
    selector->free_data = free_data;
  }

  return selector;
}

ResourceSelector* NewNotResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*)) {
  ASSERT(fn);
  ResourceSelector* selector = (ResourceSelector*)malloc(sizeof(ResourceSelector));
  if (selector) {
    selector->kind = kNotSelector;
    selector->fn = fn;
    selector->data = data;
    selector->free_data = free_data;
  }

  return selector;
}

ResourceSelector* NewNegateResourceSelector(ResourceSelector* delegate) {
  ASSERT(delegate);
  ResourceSelector* selector = (ResourceSelector*)malloc(sizeof(ResourceSelector));
  if (selector) {
    selector->kind = kNegateSelector;
    selector->selectors = (ResourceSelector**)malloc(sizeof(ResourceSelector*) * 1);
    selector->selectors[0] = delegate;
    selector->num_selectors = 1;
  }

  return selector;
}

static inline ResourceSelector* NewCompositeSelector(const ResourceSelectorKind kind, ResourceSelector** selectors,
                                                     const uint64_t num_selectors) {
  if (!selectors || num_selectors == 0)
    return NULL;

  ResourceSelector* selector = (ResourceSelector*)malloc(sizeof(ResourceSelector));
  memset(selector, 0, sizeof(ResourceSelector));
  if (selector) {
    selector->kind = kind;
    ResourceSelector** new_selectors = (ResourceSelector**)malloc(sizeof(ResourceSelector*) * num_selectors);
    if (!new_selectors) {
      free(selector);
      return NULL;
    }

    for (uint64_t i = 0; i < num_selectors; i++)
      new_selectors[i] = selectors[i];

    selector->selectors = new_selectors;
    selector->num_selectors = num_selectors;
  }

  return selector;
}

ResourceSelector* NewAndResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors) {
  return NewCompositeSelector(kAndSelector, selectors, num_selectors);
}

ResourceSelector* NewOrResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors) {
  return NewCompositeSelector(kOrSelector, selectors, num_selectors);
}

static inline bool MatchesLabel(const Resource* res, void* data) {
  if (!res || !data)
    return false;
  return ResourceHasLabel(res, *(const Label*)data);
}

static inline bool MatchesKind(const Resource* res, void* data) {
  if (!res || !data)
    return false;
  return res->kind == *((ResourceKind*)data);
}

ResourceSelector* NewKindResourceSelector(const char* rhs) {
  if (!rhs)
    return NULL;

  ResourceKindInfo* info = FindResourceKindInfo(rhs);
  if (!info)
    return NULL;

  return NewResourceSelector(&MatchesKind, &info->kind, NULL);
}

static inline bool MatchesRef(const Resource* res, void* data) {
  if (!res || !data)
    return false;
  return ResourceMatchesRef(res, (const char*)data);
}

ResourceSelector* NewRefResourceSelector(const char* rhs) {
  if (!rhs)
    return NULL;
  return NewResourceSelector(&MatchesRef, strdup(rhs), free);
}

static inline bool MatchesNamespace(const Resource* res, void* data) {
  if (!res || !data)
    return false;

  return strcmp(res->info.ns, (const char*)data) == 0;
}

ResourceSelector* NewNamespaceResourceSelector(const char* ns) {
  if (!ns)
    return NULL;

  return NewResourceSelector(&MatchesNamespace, strdup(ns), free);
}

static inline bool MatchesName(const Resource* res, void* data) {
  if (!res || !res->info.name || !data)
    return false;

  return strcmp(res->info.name, (const char*)data) == 0;
}

ResourceSelector* NewNameResourceSelector(const char* name) {
  if (!name)
    return NULL;

  return NewResourceSelector(&MatchesName, strdup(name), free);
}

static inline bool MatchesState(const Resource* res, void* data) {
  if (!res || !data)
    return false;

  return res->state == *(ResourceState*)data;
}

ResourceSelector* NewStateResourceSelector(ResourceState rhs) {
  return NewResourceSelector(&MatchesState, &rhs, NULL);
}

ResourceSelector* NewLabelResourceSelector(const Label* rhs) {
  if (!rhs)
    return NULL;

  Label* label = (Label*)malloc(sizeof(Label));
  memcpy(label, rhs, sizeof(Label));
  return NewResourceSelector(&MatchesLabel, label, free);
}

static inline bool MatchesAnnotation(const Resource* res, void* data) {
  if (!res || !data)
    return false;
  return ResourceHasAnnotation(res, (const Annotation*)data);
}

static inline bool MatchesAnnoationKey(const Resource* res, void* data) {
  if (!res || !data)
    return false;
  return ResourceHasAnnotationK(res, (const AnnotationKey*)data);
}

static inline bool MatchesAnnotationValue(const Resource* res, void* data) {
  if (!res || !data)
    return false;
  return ResourceHasAnnotationV(res, (const AnnotationValue*)data);
}

ResourceSelector* NewAnnotationResourceSelector(const Annotation* rhs) {
  if (!rhs)
    return NULL;
  Annotation* annotation = (Annotation*)malloc(sizeof(Annotation));
  memcpy(annotation, rhs, sizeof(Annotation));
  return NewResourceSelector(&MatchesAnnotation, annotation, free);
}

ResourceSelector* NewAnnotationKeyResourceSelector(const AnnotationKey* rhs) {
  if (!rhs)
    return NULL;
  AnnotationKey* key = (AnnotationKey*)malloc(sizeof(AnnotationKey));
  memcpy(key, rhs, sizeof(AnnotationKey));
  return NewResourceSelector(&MatchesAnnoationKey, key, free);
}

ResourceSelector* NewAnnotationValueResourceSelector(const AnnotationValue* rhs) {
  if (!rhs)
    return NULL;
  AnnotationValue* value = (AnnotationValue*)malloc(sizeof(AnnotationValue));
  memcpy(value, rhs, sizeof(AnnotationValue));
  return NewResourceSelector(&MatchesAnnotationValue, value, NULL);
}

#define BEGIN_FOREACH_SELECTOR(Value, Name)               \
  for (uint64_t i = 0; i < (Value)->num_selectors; i++) { \
    ResourceSelector* Name = rs->selectors[i];

#define END_FOREACH_SELECTOR }

static inline bool MatchAll(const ResourceSelector* rs, const Resource* res) {
  ASSERT(rs);
  ASSERT_EQ(rs->kind, kAndSelector);
  ASSERT(res);

  if (rs->num_selectors == 0)
    return false;

  BEGIN_FOREACH_SELECTOR(rs, selector)
  if (!ResourceSelectorMatch(selector, res))
    return false;
  END_FOREACH_SELECTOR;
  return true;
}

static inline bool MatchAny(const ResourceSelector* rs, const Resource* res) {
  ASSERT(rs);
  ASSERT_EQ(rs->kind, kOrSelector);
  ASSERT(res);

  if (rs->num_selectors == 0)
    return false;

  BEGIN_FOREACH_SELECTOR(rs, selector)
  if (ResourceSelectorMatch(selector, res))
    return true;
  END_FOREACH_SELECTOR;
  return false;
}

static inline bool Matches(const ResourceSelector* rs, const Resource* res) {
  ASSERT(rs);
  ASSERT(res);
  return rs->fn(res, rs->data);
}

bool ResourceSelectorMatch(const ResourceSelector* rs, const Resource* res) {
  if (!rs || !res)
    return false;

  switch (rs->kind) {
    case kAtomicSelector:
      return Matches(rs, res);
    case kNotSelector:
      return !Matches(rs, res);
    case kNegateSelector:
      return !ResourceSelectorMatch(rs->selectors[0], res);
    case kAndSelector:
      return MatchAll(rs, res);
    case kOrSelector:
      return MatchAny(rs, res);
    default:
      return false;
  }
}

void FreeResourceSelector(ResourceSelector* rs) {
  if (!rs)
    return;

  if (IsAtomicSelector(rs)) {
    if (rs->free_data && rs->data)
      rs->free_data(rs->data);
  } else if (IsCompositeSelector(rs) || IsNegateSelector(rs) || IsNotSelector(rs)) {
    if (rs->selectors && rs->num_selectors > 0) {
      BEGIN_FOREACH_SELECTOR(rs, selector)
      if (IsCompositeSelector(selector))
        FreeResourceSelector(selector);
      END_FOREACH_SELECTOR;

      free(rs->selectors);
    }
  }

  free(rs);
}

static inline void EnsureCap(ResourceSelectorBuilder* builder, const size_t new_len) {
  ASSERT(builder);
  if (new_len < builder->selectors_cap)
    return;

  const size_t new_cap = new_len;
  const size_t total_size = sizeof(ResourceSelector*) * new_cap;
  ResourceSelector** new_selectors = (ResourceSelector**)realloc(builder->selectors, total_size);
  LOG_FATAL_IF(!new_selectors, "failed to reallocate new ResourceSelector array of %zu items", new_len);
  builder->selectors = new_selectors;
  builder->selectors_cap = new_cap;
}

void InitResourceSelectorBuilder(ResourceSelectorBuilder* builder, const size_t init_cap) {
  ASSERT(builder);
  if (init_cap == 0) {
    builder->selectors = NULL;
    builder->selectors_len = builder->selectors_cap = 0;
    return;
  }

  memset(builder, 0, sizeof(ResourceSelectorBuilder));
  EnsureCap(builder, init_cap);
}

ResourceSelector* BuildAndResourceSelector(ResourceSelectorBuilder* builder) {
  ASSERT(builder);
  ASSERT(builder->selectors);
  ASSERT(builder->selectors_len > 0);
  ResourceSelector* selector = NewAndResourceSelector(builder->selectors, builder->selectors_len);

  free(builder->selectors);
  builder->selectors = NULL;
  builder->selectors_len = builder->selectors_cap = 0;

  return selector;
}

ResourceSelector* BuildOrResourceSelector(ResourceSelectorBuilder* builder) {
  ASSERT(builder);
  ASSERT(builder->selectors);
  ASSERT(builder->selectors_len > 0);
  ResourceSelector* selector = NewOrResourceSelector(builder->selectors, builder->selectors_len);

  free(builder->selectors);
  builder->selectors = NULL;
  builder->selectors_len = builder->selectors_cap = 0;

  return selector;
}

void AppendResourceSelectors(ResourceSelectorBuilder* builder, ResourceSelector** selectors,
                             const size_t num_selectors) {
  ASSERT(builder);
  ASSERT(selectors);
  ASSERT(num_selectors > 0);
  EnsureCap(builder, builder->selectors_len + num_selectors);
  const size_t total_size = sizeof(ResourceSelector*) * num_selectors;
  memcpy(&builder->selectors[builder->selectors_len], selectors, total_size);
  builder->selectors_len += num_selectors;
}

void FreeResourceSelectorBuilder(ResourceSelectorBuilder* builder) {
  ASSERT(builder);
  if (builder->selectors) {
    for (size_t i = 0; i < builder->selectors_len; i++)
      FreeResourceSelector(builder->selectors[i]);
    free(builder->selectors);
  }
}
