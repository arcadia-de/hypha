#ifndef HYPHA_ORCHESTRATOR_H
#define HYPHA_ORCHESTRATOR_H

#include <lua.h>

#include "hypha.h"
#include "hypha/event.h"
#include "hypha/resource_graph.h"

typedef struct _Orchestrator Orchestrator;

typedef struct {
  char* root;
  char* state_dir;
  char* cache_dir;
} OrchestratorConfig;

typedef struct {
  bool success;
  char* message;
} EvalResult;

OrchestratorHandle NewOrchestrator(OrchestratorConfig config);
ResourceGraph* OrchestratorGetResourceGraph(OrchestratorHandle);
HistoryLog* OrchestratorGetHistoryLog(OrchestratorHandle);
lua_State* OrchestratorGetLuaState(OrchestratorHandle);
EventBus* OrchestratorGetEventBus(OrchestratorHandle);
void OrchestratorRenderResourceGraphTo(OrchestratorHandle, const char* name, const char* layout, const char* render,
                                       FILE* stream);
void OrchestratorAddResource(OrchestratorHandle, Resource);
void OrchestratorOnEvent(OrchestratorHandle, const OrchestratorEventKind, OrchestratorEventHandlerFn, void* data);
void OrchestratorPublish(OrchestratorHandle, const OrchestratorEvent*);
bool OrchestratorRun(OrchestratorHandle);
bool OrchestratorPruneOrphans(OrchestratorHandle);
bool OrchestratorCompact(OrchestratorHandle);
bool OrchestratorEvalExpr(OrchestratorHandle, const char* expr, char** err);
bool OrchestratorEvalFile(OrchestratorHandle, const char* filename, char** err);
void FreeOrchestrator(OrchestratorHandle);
void OrchestratorPrintRuntimeInfo(OrchestratorHandle orc);

static inline void OrchestratorRenderResourceGraphToStdout(OrchestratorHandle handle, const char* name,
                                                           const char* layout, const char* render) {
  return OrchestratorRenderResourceGraphTo(handle, name, layout, render, stdout);
}

static inline void OrchestratorPublishGraphSubmitted(OrchestratorHandle handle) {
  OrchestratorEvent event = {
      .kind = kGraphSubmittedEvent,
      .action = kNoAction,
      .status = kStatusOk,
      .resource = NULL,  // NOLINT(modernize-use-nullptr)
  };
  return OrchestratorPublish(handle, &event);
}

#define DEFINE_ON_EVENT(Name)                                                                                     \
  static inline void OrchestratorOn##Name(OrchestratorHandle handle, OrchestratorEventHandlerFn fn, void* data) { \
    return OrchestratorOnEvent(handle, k##Name##Event, fn, data);                                                 \
  }
FOR_EACH_ORCHESTRATOR_EVENT(DEFINE_ON_EVENT)
#undef DEFINE_ON_EVENT
// ──────────────────────────────────────────────────────────────────────

#endif  // HYPHA_ORCHESTRATOR_H
