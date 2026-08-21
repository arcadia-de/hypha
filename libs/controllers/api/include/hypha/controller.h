#ifndef HYPHA_CONTROLLER_H
#define HYPHA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdio.h>

#include "hypha.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

// init controller
typedef void (*ControllerInitFn)(void* data);
// de-init controller
typedef void (*ControllerDeInitFn)(void* data);
// discover changes
typedef ControllerStatus (*ControllerObserveFn)(const Resource*, Resource*, void*);
// normalize the changes with standardized metadata
typedef ControllerStatus (*ControllerNormalizeFn)(const Resource*, void*);
// reject malformed specs before planning
typedef bool (*ControllerValidateFn)(const Resource*, ValidationLog* log, void*);
// compute the change set
typedef ControllerAction (*ControllerPlanFn)(const Resource*, const Resource*, Plan*, void*);
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
  static inline ControllerAction Name##Plan(const Resource* current, const Resource* desired, Plan* pl, void* data)

#define DEFINE_CONTROLLER_APPLY_FN(Name) \
  static inline ControllerStatus Name##Apply(const Resource* desired, const ControllerAction action, void* data)

#define DEFINE_CONTROLLER_DESTROY_FN(Name) \
  static inline ControllerStatus Name##Destroy(const Resource* current, void* data)

#define DEFINE_CONTROLLER_STATUS_FN(Name) \
  static inline ControllerStatus Name##Status(const Resource* current, void* data)

#define DEFINE_CONTROLLER_VALIDATE_FN(Name) \
  static inline bool Name##Validate(const Resource* desired, ValidationLog* vlog, void* data)
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

uint32_t GetNumberOfRegisteredControllers();
Controller* GetControllerAt(const uint32_t i);

typedef bool (*ControllerVisitFn)(const uint32_t, const char*, const Controller*, void* data);
bool VisitAllControllers(ControllerVisitFn, void*);

Controller* GetControllerForKind(const char* kind);
Controller* RegisterController(const char* kind, ControllerConfig config, void* data, void (*free_data)(void*));
const char* GetControllerKind(const Controller* ctrl);

void ControllerInit(Controller*);
void ControllerDeInit(Controller*);
void ControllerObserve(Controller* ctrl, const Resource* desired, Resource* res);
ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, const Resource* desired, Plan* pl);
ControllerStatus ControllerApply(Controller* ctrl, const Resource* current, const ControllerAction action);
bool ControllerValidate(Controller* ctrl, const Resource* current, ValidationLog* vl);
ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current);
ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current);
ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current);
ControllerStatus ControllerStat(Controller* ctrl, const Resource* current);
ControllerStatus ControllerNormalize(Controller* ctrl, const Resource* current);

static inline bool IsControllerForKind(Controller* lhs, const char* rhs) {
  if (!lhs || !rhs)
    return false;

  return strcmp(GetControllerKind(lhs), rhs) == 0;
}

static inline bool HasControllerForKind(const char* rhs) {
  return GetControllerForKind(rhs) != NULL;  // NOLINT(modernize-use-nullptr)
}

#ifdef HYPHA_DEBUG
void ListRegisteredControllers(FILE* out);
#endif  // HYPHA_DEBUG

#define DEFINE_NEW_CONTROLLER(Name, Kind)                                     \
  Controller* New##Name##Controller() {                                       \
    return RegisterController((Kind), k##Name##ControllerConfig, NULL, NULL); \
  }

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_CONTROLLER_H
