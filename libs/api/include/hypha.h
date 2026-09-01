#ifndef HYPHA_H
#define HYPHA_H

#include "hypha/controller_status.h"
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdlib.h>

#include "hypha/controller_action.h"

#define LUA_REGISTRY_ORC_KEY    "hypha_orchestrator"
#define LUA_REGISTRY_EVENTS_KEY "hypha_events"

#ifndef container_of
#define container_of(ptr, type, member)               \
  ({                                                  \
    const typeof(((type*)0)->member)* __mptr = (ptr); \
    (type*)((char*)__mptr - offsetof(type, member));  \
  })
#endif  // container_of

typedef struct _Resource Resource;

typedef void* OrchestratorHandle;

typedef uint64_t ControllerActionCounts[kTotalNumberOfControllerActions];
typedef uint64_t ControllerStatusCounts[kTotalNumberOfControllerStatuses];

typedef struct {
  time_t run_start;
  time_t run_finished;
  uint64_t num_processed;
  ControllerActionCounts num_actions;
  ControllerStatusCounts num_statuses;
} OrchestratorMetrics;

typedef struct _HistoryLog HistoryLog;

void InitHypha(const char* luarocks_dir);
extern char* RenderJsonnet(char* name, char* code);
extern char* RenderTemplate(char* tpl, char* data, bool is_yaml);
extern uint64_t ValidateManifests(char** tpls, uint64_t num_tpls, bool* valid);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_H
