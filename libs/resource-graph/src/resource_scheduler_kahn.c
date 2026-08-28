#include <strings.h>

#include "hypha/annotation.h"
#include "hypha/assertions.h"
#include "hypha/log.h"
#include "hypha/resource_bootstrap.h"
#include "hypha/resource_graph.h"

static inline ResourceGraphIndex FindResourceIndexByRef(const Resource* resources, const size_t num_resources,
                                                        const char* ref) {
  if (!ref)
    return kInvalidResourceIndex;

  for (size_t i = 0; i < num_resources; i++) {
    if (ResourceMatchesRef(&resources[i], ref))
      return (ResourceGraphIndex)i;
  }

  return kInvalidResourceIndex;
}

static ResourceGraphIndex** ResolveExplicitDependencies(const Resource* resources, const size_t num_resources) {
  ResourceGraphIndex** resolved = (ResourceGraphIndex**)calloc(num_resources, sizeof(ResourceGraphIndex*));
  if (!resolved)
    return NULL;

  for (size_t i = 0; i < num_resources; i++) {
    const Resource* res = &resources[i];
    if (res->num_depends_on == 0)
      continue;

    resolved[i] = (ResourceGraphIndex*)malloc(sizeof(ResourceGraphIndex) * res->num_depends_on);
    if (!resolved[i])
      goto failed;

    for (size_t j = 0; j < res->num_depends_on; j++) {
      const ResourceGraphIndex dep_idx = FindResourceIndexByRef(resources, num_resources, res->depends_on[j]);
      if (dep_idx == kInvalidResourceIndex) {
        LOG_ERROR("resource '%s' depends on unknown resource '%s'", res->info.name ? res->info.name : "<unnamed>",
                  res->depends_on[j]);
        goto failed;
      }

      resolved[i][j] = dep_idx;
    }
  }

  return resolved;
failed:
  for (size_t i = 0; i < num_resources; i++)
    free(resolved[i]);
  free(resolved);
  return NULL;
}

static void FreeExplicitDependencies(ResourceGraphIndex** resolved, const size_t num_resources) {
  if (!resolved)
    return;

  for (size_t i = 0; i < num_resources; i++)
    free(resolved[i]);
  free(resolved);
}

static inline void PropagatePriorityToDependencies(Resource* resources, const size_t num_resources,
                                                   ResourceGraphIndex* const* explicit_deps) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < num_resources; i++) {
      const Resource* res = &resources[i];
      for (size_t j = 0; j < res->num_depends_on; j++) {
        Resource* dep = &resources[explicit_deps[i][j]];
        if (res->priority > dep->priority) {
          dep->priority = res->priority;
          changed = true;
        }
      }
    }
  }
}

typedef enum {
  kSchedulingTierController = 0,
  kSchedulingTierPackageManager,
  kSchedulingTierPackage,
  kSchedulingTierDefault,
} SchedulingTier;

typedef struct {
  ResourceKind controller;
  ResourceKind package_manager;
  ResourceKind package;
} SchedulingTierKinds;

static inline SchedulingTierKinds ResolveSchedulingTierKinds() {
  return (SchedulingTierKinds){
      .controller = FindResourceKind("Controller"),
      .package_manager = FindResourceKind("PackageManager"),
      .package = FindResourceKind("Package"),
  };
}

static inline SchedulingTier GetSchedulingTier(const SchedulingTierKinds* kinds, const Resource* res) {
  if (res->kind == kinds->controller)
    return kSchedulingTierController;
  else if (res->kind == kinds->package_manager)
    return kSchedulingTierPackageManager;
  else if (res->kind == kinds->package)
    return kSchedulingTierPackage;

  return kSchedulingTierDefault;
}

static inline const char* GetDeclaredPackageManagerName(const Resource* res) {
  if (!res->spec.doc)
    return NULL;

  json_t* manager = json_object_get(res->spec.doc, "manager");
  if (!manager || !json_is_string(manager))
    return NULL;

  return json_string_value(manager);
}

static inline ResourceGraphIndex FindPackageManagerProviding(const Resource* resources, const size_t num_resources,
                                                             const ResourceKind package_manager_kind,
                                                             const char* provides) {
  for (size_t i = 0; i < num_resources; i++) {
    const Resource* res = &resources[i];
    if (res->kind != package_manager_kind)
      continue;

    Annotation* found = NULL;
    if (!ResourceGetAnnotation(res, &kProvidesAnnotationKey, &found) || !found)
      continue;

    if (strcasecmp(found->value, provides) == 0)
      return (ResourceGraphIndex)i;
  }

  return kInvalidResourceIndex;
}

static int64_t* ResolvePackageManagers(const Resource* resources, const size_t num_resources,
                                       const SchedulingTierKinds* tier_kinds, uint32_t* in_degree) {
  int64_t* package_manager_idx = (int64_t*)malloc(sizeof(int64_t) * num_resources);
  if (!package_manager_idx)
    return NULL;

  for (size_t i = 0; i < num_resources; i++) {
    package_manager_idx[i] = -1;

    const Resource* res = &resources[i];
    if (GetSchedulingTier(tier_kinds, res) != kSchedulingTierPackage)
      continue;

    const char* provides = GetDeclaredPackageManagerName(res);
    if (!provides)
      continue;

    const ResourceGraphIndex mgr_idx =
        FindPackageManagerProviding(resources, num_resources, tier_kinds->package_manager, provides);
    if (mgr_idx == kInvalidResourceIndex) {
      LOG_WARN("package '%s' declares manager '%s' but no PackageManager provides it -- scheduling ungrouped",
               res->info.name ? res->info.name : "<unnamed>", provides);
      continue;
    }

    package_manager_idx[i] = (int64_t)mgr_idx;
    in_degree[i]++;
  }

  return package_manager_idx;
}

bool ComputeSchedulePriorityWeightedKahn(Resource* resources, const size_t num_resources,
                                         ResourceGraphIndex** results) {
  if (!resources || num_resources == 0)
    return true;

  bool ok = false;
  ResourceGraphIndex** explicit_deps = NULL;
  uint32_t* in_degree = NULL;
  int64_t* package_manager_idx = NULL;
  int64_t* manager_rank = NULL;
  bool* scheduled = NULL;
  ResourceGraphIndex* order = NULL;

  explicit_deps = ResolveExplicitDependencies(resources, num_resources);
  if (!explicit_deps)
    goto finished;

  PropagatePriorityToDependencies(resources, num_resources, (ResourceGraphIndex* const*)explicit_deps);

  const SchedulingTierKinds tier_kinds = ResolveSchedulingTierKinds();

  in_degree = (uint32_t*)malloc(sizeof(uint32_t) * num_resources);
  if (!in_degree)
    goto finished;
  for (size_t i = 0; i < num_resources; i++)
    in_degree[i] = (uint32_t)resources[i].num_depends_on;

  package_manager_idx = ResolvePackageManagers(resources, num_resources, &tier_kinds, in_degree);
  if (!package_manager_idx)
    goto finished;

  scheduled = (bool*)calloc(num_resources, sizeof(bool));
  order = (ResourceGraphIndex*)malloc(sizeof(ResourceGraphIndex) * num_resources);
  manager_rank = (int64_t*)malloc(sizeof(int64_t) * num_resources);
  if (!scheduled || !order || !manager_rank)
    goto finished;
  for (size_t i = 0; i < num_resources; i++)
    manager_rank[i] = -1;

  uint32_t output_len = 0;
  while (output_len < num_resources) {
    int64_t best = -1;
    SchedulingTier best_tier = kSchedulingTierDefault;
    int64_t best_rank = INT64_MAX;

    for (size_t i = 0; i < num_resources; i++) {
      if (scheduled[i] || in_degree[i] != 0)
        continue;

      const SchedulingTier tier = GetSchedulingTier(&tier_kinds, &resources[i]);
      const int64_t rank = (tier == kSchedulingTierPackage && package_manager_idx[i] >= 0)
                             ? manager_rank[(size_t)package_manager_idx[i]]
                             : INT64_MAX;

      bool better = false;
      if (best == -1)
        better = true;
      else if (tier != best_tier)
        better = tier < best_tier;
      else if (tier == kSchedulingTierPackage && rank != best_rank)
        better = rank < best_rank;
      else
        better = resources[i].priority > resources[(size_t)best].priority;

      if (better) {
        best = (int64_t)i;
        best_tier = tier;
        best_rank = rank;
      }
    }

    if (best == -1) {
      LOG_ERROR("failed to compute schedule: dependency cycle detected among %zu remaining resource(s)",
                num_resources - output_len);
      goto finished;
    }

    const size_t best_idx = (size_t)best;
    order[output_len] = (ResourceGraphIndex)best_idx;
    scheduled[best_idx] = true;
    if (best_tier == kSchedulingTierPackageManager)
      manager_rank[best_idx] = output_len;
    output_len++;

    for (size_t i = 0; i < num_resources; i++) {
      if (scheduled[i])
        continue;

      bool depends_on_best = (package_manager_idx[i] == (int64_t)best_idx);
      for (size_t j = 0; !depends_on_best && j < resources[i].num_depends_on; j++) {
        if (explicit_deps[i][j] == best_idx)
          depends_on_best = true;
      }

      if (depends_on_best)
        in_degree[i]--;
    }
  }

  (*results) = order;
  order = NULL;
  ok = true;

finished:
  FreeExplicitDependencies(explicit_deps, num_resources);
  free(in_degree);
  free(package_manager_idx);
  free(manager_rank);
  free(scheduled);
  free(order);
  return ok;
}
