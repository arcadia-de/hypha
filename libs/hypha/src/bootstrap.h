#ifndef HYPHA_INTERNAL_BOOTSTRAP_H
#define HYPHA_INTERNAL_BOOTSTRAP_H

#include "hypha/resource_graph.h"

// Builds the concrete CoreResourceDef table -- one Controller entry per statically-linked
// controller (see FOR_EACH_CONTROLLER in hypha.c), one PackageBackend entry per
// statically-linked package-manager backend (see FOR_EACH_PACKAGE_MANAGER in
// package_manager.h) -- and injects it into `graph` via BootstrapCoreResources.
//
// Must be called exactly once, immediately after the graph is constructed and before any
// manifest-sourced resource is added -- see NewOrchestrator in orchestrator_init.c. This is
// the composition root for "what compiled-in resources does this binary have": it's the one
// place allowed to know about both the controllers lib and the pkg-manager lib, so that
// resource-graph itself (BootstrapCoreResources) stays free of a dependency on either.
void BootstrapHyphaCoreResources(ResourceGraph* graph);

#endif  // HYPHA_INTERNAL_BOOTSTRAP_H
