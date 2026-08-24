#ifndef HYPHA_CONTROLLER_H
#define HYPHA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdio.h>

#include "hypha.h"
#include "hypha/controller_status.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/resource_kind.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

// init controller
typedef void (*ControllerInitFn)(void* data);
// de-init controller
typedef void (*ControllerDeInitFn)(void* data);
// discover changes
typedef ControllerStatus (*ControllerObserveFn)(const Resource* current, Resource* desired, void*);
// normalize the changes with standardized metadata
typedef ControllerStatus (*ControllerNormalizeFn)(const Resource*, void*);
// reject malformed specs before planning
typedef bool (*ControllerValidateFn)(Resource*, ValidationLog* log, void*);
// compute the change set
typedef ControllerAction (*ControllerPlanFn)(const Resource* current, Resource* desired, Plan*, void*);
// execute the changes
typedef ControllerStatus (*ControllerApplyFn)(const Resource*, const ControllerAction, void*);
// remove the resources cleanly
typedef ControllerStatus (*ControllerDestroyFn)(const Resource*, void*);
// report the changes between current and planned
typedef ControllerStatus (*ControllerDiffFn)(const Resource*, void*);
// report current state and drift
typedef ControllerStatus (*ControllerStatusFn)(const Resource*, void*);
// rollback changes
typedef ControllerStatus (*ControllerRollbackFn)(const Resource*, void*);

// clang-format off
#define DEFINE_CONTROLLER_INIT_FN(Name) \
  static inline void Name##Init(void* data)

#define DEFINE_CONTROLLER_DEINIT_FN(Name) \
  static inline void Name##DeInit(void* data)

#define DEFINE_CONTROLLER_OBSERVE_FN(Name) \
  static inline ControllerStatus Name##Observe(const Resource* desired, Resource* out, void* data)

#define DEFINE_CONTROLLER_PLAN_FN(Name) \
  static inline ControllerAction Name##Plan(const Resource* current, Resource* desired, Plan* pl, void* data)

#define DEFINE_CONTROLLER_APPLY_FN(Name) \
  static inline ControllerStatus Name##Apply(const Resource* desired, const ControllerAction action, void* data)

#define DEFINE_CONTROLLER_DESTROY_FN(Name) \
  static inline ControllerStatus Name##Destroy(const Resource* current, void* data)

#define DEFINE_CONTROLLER_STATUS_FN(Name) \
  static inline ControllerStatus Name##Status(const Resource* current, void* data)

#define DEFINE_CONTROLLER_VALIDATE_FN(Name) \
  static inline bool Name##Validate(Resource* desired, ValidationLog* vlog, void* data)
// clang-format on

typedef struct {
  ControllerInitFn init;
  ControllerDeInitFn deinit;
  ControllerObserveFn observe;
  ControllerPlanFn plan;
  ControllerApplyFn apply;
  ControllerDestroyFn destroy;
  ControllerValidateFn validate;
  ControllerDiffFn diff;
  ControllerStatusFn status;
  ControllerRollbackFn rollback;
  ControllerNormalizeFn normalize;
} ControllerConfig;

typedef struct _Controller Controller;

uint64_t GetNumberOfRegisteredControllers();
Controller* GetControllerAt(const uint64_t i);
void FreeAllControllers();

typedef bool (*ControllerVisitFn)(const uint32_t, const char*, const Controller*, void* data);
bool VisitAllControllers(ControllerVisitFn, void*);

Controller* GetControllerForKind(ResourceKind kind);
Controller* GetControllerForKindName(const char* name);
Controller* NewController(ResourceKind kind, ControllerConfig config, void* data, void (*free_data)(void*));
ResourceKind GetControllerKind(const Controller* ctrl);

void ControllerInit(Controller*);
void ControllerDeInit(Controller*);
ControllerStatus ControllerObserve(Controller* ctrl, const Resource* current, Resource* desired);
ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, Resource* desired, Plan* pl);
ControllerStatus ControllerApply(Controller* ctrl, const Resource* desired, const ControllerAction action);
bool ControllerValidate(Controller* ctrl, Resource* current, ValidationLog* vl);
ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current);
ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current);
ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current);
ControllerStatus ControllerStat(Controller* ctrl, const Resource* current);
ControllerStatus ControllerNormalize(Controller* ctrl, const Resource* current);

#define DEFINE_NEW_CONTROLLER(Name, Kind)                                       \
  static ResourceKind k##Name##Kind = kInvalidResourceKind;                     \
  Controller* New##Name##Controller() {                                         \
    k##Name##Kind = NewResourceKind((Kind));                                    \
    if (k##Name##Kind == kInvalidResourceKind)                                  \
      return NULL;                                                              \
    return NewController(k##Name##Kind, k##Name##ControllerConfig, NULL, NULL); \
  }

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_CONTROLLER_H
