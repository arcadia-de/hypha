#ifndef HYPHA_RESOURCE_BOOTSTRAP_H
#define HYPHA_RESOURCE_BOOTSTRAP_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stddef.h>

#include "hypha/resource_flags.h"
#include "hypha/resource_graph.h"
#include "hypha/resource_kind.h"
#include "hypha/resource_namespace.h"

// Describes one compiled-in resource to inject into a fresh ResourceGraph before any
// manifest is parsed.
//
// This is deliberately generic over *what* is being bootstrapped -- the same mechanism
// backs core Controller resources (kind="Controller", provides="Symlink", ...) and core
// PackageBackend resources (kind="PackageBackend", provides="Brew", ...) via a single call
// to BootstrapCoreResources. The concrete table is owned by the caller (see
// BootstrapHyphaCoreResources in libs/hypha/src/bootstrap.c), so this library never takes a
// dependency on the controllers or pkg-manager libs -- it only knows how to place
// pre-described resources into a graph.
typedef struct {
  const char* kind;
  const char* name;      // stable name; combined with ns+kind for deterministic identity
  const char* ns;        // namespace; NULL defaults to kCoreResourceNamespace
  const char* provides;  // optional: recorded as the "hypha/provides" annotation, used to
                         // look up/dedupe provider-style resources by capability rather
                         // than by name (see FindResourceProviding); NULL to omit
  ResourceFlags flags;   // typically kResourceFlagStatic; caller decides
} CoreResourceDef;

// Injects one Resource per entry in `defs` directly into `graph`, bypassing the normal
// "freshly pending" contract of AllocNewResouceInGraph: bootstrap resources are created
// already in kResourceReady state, so the scheduler and reconcile dispatch loop
// (QueueReconcileTaskForResource / DependenciesAreSatisfied) skip them as a matter of
// course -- they never occupy a wavefront slot, and are implicitly satisfied as soon as
// anything else names them as a dependency. They never run through
// observe/normalize/plan/apply.
//
// Resource id is derived deterministically (UUIDv5 over namespace+kind+name, not random)
// so a given core resource has the same id on every process run -- required for
// depends_on-by-id references and for state/history correlation to stay stable across
// runs. Two defs that resolve to the same (ns, kind, name) therefore collide by design;
// that's a caller bug (a duplicate table entry), not something this function silently
// merges.
//
// Call once, immediately after NewResourceGraph(), before any manifest-sourced resource is
// added. Returns false on the first allocation failure; whatever was injected before the
// failure remains in the graph.
bool BootstrapCoreResources(ResourceGraph* graph, const CoreResourceDef* defs, const size_t num_defs);

// Looks up a resource of `kind` whose "hypha/provides" annotation equals `provides`.
//
// This is the identity/dedup primitive for provider-style resources: two resources of the
// same kind must not claim to provide the same capability (e.g. two Controller resources
// both declaring provides="Symlink", or a manifest-declared PackageBackend colliding with
// a compiled-in one). Callers -- the controller-controller's validate stage, or normalize's
// implicit-edge synthesis for a Package resource's backend -- use this both to reject
// conflicting duplicates and to find the existing node an implicit dependency edge should
// point at, instead of instantiating a fresh one.
bool FindResourceProviding(ResourceGraph* graph, const char* kind, const char* provides, Resource** out);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RESOURCE_BOOTSTRAP_H
