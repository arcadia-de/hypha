#ifndef HYPHA_ORCHESTRATOR_H
#define HYPHA_ORCHESTRATOR_H

#include <lua.h>

#include "hypha.h"
#include "hypha/event.h"
#include "hypha/planner.h"
#include "hypha/resource_graph.h"

typedef struct _Orchestrator Orchestrator;

void GetOrchestratorMetrics(OrchestratorHandle orc, OrchestratorMetrics* out);

typedef struct {
  char* root;
  char* state_dir;
  char* cache_dir;
} OrchestratorConfig;

typedef struct {
  bool success;
  char* message;
} EvalResult;

typedef struct {
  OrchestratorRunMode mode;
  char* id;
  Reason reason;
} OrchestratorRunConfig;

OrchestratorHandle NewOrchestrator(OrchestratorConfig config);
ResourceGraph* OrchestratorGetResourceGraph(OrchestratorHandle);
HistoryLog* OrchestratorGetHistoryLog(OrchestratorHandle);
lua_State* OrchestratorGetLuaState(OrchestratorHandle);
EventRoute* GetOrchestratorRootEventRoute(OrchestratorHandle);
EventBus* OrchestratorGetEventBus(OrchestratorHandle);

void OrchestratorAddResource(OrchestratorHandle, Resource);
void OrchestratorSubscribe(OrchestratorHandle, const char* p, EventCallbackFn cb, void* data, void (*free_data)(void*));
void OrchestratorPublish(OrchestratorHandle, const char* p, void* event);
bool OrchestratorRunWithReason(OrchestratorHandle, const OrchestratorRunMode mode, const Reason reason);
bool OrchestratorPruneOrphans(OrchestratorHandle);
bool OrchestratorCompact(OrchestratorHandle);
bool OrchestratorEvalExpr(OrchestratorHandle, const char* expr, char** err);
bool OrchestratorEvalFile(OrchestratorHandle, const char* filename, char** err);
void FreeOrchestrator(OrchestratorHandle);
void OrchestratorPrintRuntimeInfo(OrchestratorHandle);
Plan* GetOrchestratorPlan(OrchestratorHandle);

static inline bool OrchestratorRun(OrchestratorHandle handle, const OrchestratorRunMode mode) {
  Reason reason;
  memset(reason, '\0', sizeof(Reason));
  return OrchestratorRunWithReason(handle, mode, reason);
}

#ifdef HYPHA_GRAPHVIZ_ENABLED
void OrchestratorRenderResourceGraphTo(OrchestratorHandle, const char* name, const char* layout, const char* render,
                                       FILE* stream);

static inline void OrchestratorRenderResourceGraphToStdout(OrchestratorHandle handle, const char* name,
                                                           const char* layout, const char* render) {
  return OrchestratorRenderResourceGraphTo(handle, name, layout, render, stdout);
}
#endif  // HYPHA_GRAPHVIZ_ENABLED

#endif  // HYPHA_ORCHESTRATOR_H
