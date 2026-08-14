#ifdef HYPHA_GRAPHVIZ_ENABLED

#include "hypha/orchestrator.h"

void OrchestratorRenderResourceGraphTo(OrchestratorHandle handle, const char* name, const char* layout,
                                       const char* render, FILE* stream) {
  ASSERT(handle);
  ASSERT(name);
  ASSERT(layout);
  ASSERT(render);
  ASSERT(stream);
}

#endif  // HYPHA_GRAPHVIZ_ENABLED
