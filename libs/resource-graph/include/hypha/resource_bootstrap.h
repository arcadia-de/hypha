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

typedef struct {
  const char* kind;
  const char* name;
  const char* ns;
  const char* provides;
  const char* docs;

  ResourceFlags flags;
} CoreResourceDef;

bool BootstrapCoreResources(ResourceGraph* graph, const CoreResourceDef* defs, const size_t num_defs);
bool FindResourceProviding(ResourceGraph* graph, const char* kind, const char* provides, Resource** out);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RESOURCE_BOOTSTRAP_H
