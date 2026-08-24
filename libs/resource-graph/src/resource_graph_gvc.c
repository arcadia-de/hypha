#ifdef HYPHA_GRAPHVIZ_ENABLED

#include "hypha.h"
#include "hypha/assertions.h"
#include "hypha/log.h"
#include "hypha/resource_graph.h"

bool ResourceGraphToGraphviz(const ResourceGraph* rg, const char* name, Agraph_t** out) {
  bool success = false;
  if (!rg)
    goto failed;

  Agraph_t* g = agopen((char*)name, Agdirected, NULL);
  if (!g)
    goto failed;

  agnode(g, "A", 1);
  agnode(g, "B", 1);
  agedge(g, agfindnode(g, "A"), agfindnode(g, "B"), 0, 1);

  (*out) = g;
  success = true;
  goto finished;
failed:
  (*out) = NULL;
finished:
  return success;
}

void RenderResourceGraphToGraphvizWithLayout(const ResourceGraph* rg, const char* name, const char* layout,
                                             const char* render, FILE* out) {
  ASSERT(rg);
  ASSERT(name);
  ASSERT(layout);
  ASSERT(render);
  GVC_t* gvc = gvContext();
  if (!gvc)
    goto finished;

  Agraph_t* g = NULL;
  if (!ResourceGraphToGraphviz(rg, name, &g)) {
    LOG_ERROR("failed to convert ResourceGraph to graphviz");
    goto failed0;
  }

  gvLayout(gvc, g, layout);
  gvRender(gvc, g, render, out);

  gvFreeLayout(gvc, g);
  agclose(g);
failed0:
  gvFreeContext(gvc);
finished:
  return;
}

#endif  // HYPHA_GRAPHVIZ_ENABLED
