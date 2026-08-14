#ifndef HYPHA_CONTROLLER_H
#define HYPHA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdio.h>

#include "hypha.h"
#include "hypha/resource.h"

#define FOR_EACH_CONTROLLER_VALIDATION_RESULT(V) \
  V(kFailed)                                     \
  V(kPassed)                                     \
  V(kSkipped)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) kValidation##Name,
  FOR_EACH_CONTROLLER_VALIDATION_RESULT(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfControllerValidationResults,
} ControllerValidationResult;
// clang-format on

// init controller
typedef void (*ControllerInitFn)(void* data);

// de-init controller
typedef void (*ControllerDeInitFn)(void* data);

// discover changes
typedef ControllerStatus (*ControllerObserveFn)(const Resource* desired, Resource* out, void* data);

// normalize the changes with standardized metadata
typedef ControllerStatus (*ControllerNormalizeFn)(const Resource*, void* data);

// reject malformed specs before planning
typedef ControllerValidationResult (*ControllerValidateFn)(const Resource*, Reason reason, void* data);

// compute the change set
typedef ControllerAction (*ControllerPlanFn)(const Resource*, const Resource*, Reason reason, void* data);

// execute the changes
typedef ControllerStatus (*ControllerApplyFn)(const Resource*, const ControllerAction, void* data);

// remove the resources cleanly
typedef ControllerStatus (*ControllerDestroyFn)(const Resource*, void* data);

// report the changes between current and planned
typedef ControllerStatus (*ControllerDiffFn)(const Resource*, void* data);

// report current state and drift
typedef ControllerStatus (*ControllerStatusFn)(const Resource*, void* data);

// rollback changes
typedef ControllerStatus (*ControllerRollbackFn)(const Resource*, void* data);

// clang-format off
#define DEFINE_CONTROLLER_INIT_FN(Name) \
  static inline void Name##Init(void* data)

#define DEFINE_CONTROLLER_DEINIT_FN(Name) \
  static inline void Name##DeInit(void* data)

#define DEFINE_CONTROLLER_OBSERVE_FN(Name) \
  static inline ControllerStatus Name##Observe(const Resource* desired, Resource* out, void* data)

#define DEFINE_CONTROLLER_PLAN_FN(Name) \
  static inline ControllerAction Name##Plan(const Resource* current, const Resource* desired, Reason reason, void* data)

#define DEFINE_CONTROLLER_APPLY_FN(Name) \
  static inline ControllerStatus Name##Apply(const Resource* desired, const ControllerAction action, void* data)

#define DEFINE_CONTROLLER_DESTROY_FN(Name) \
  static inline ControllerStatus Name##Destroy(const Resource* current, void* data)

#define DEFINE_CONTROLLER_STATUS_FN(Name) \
  static inline ControllerStatus Name##Status(const Resource* current, void* data)

#define DEFINE_CONTROLLER_VALIDATE_FN(Name) \
  static inline ControllerValidationResult Name##Validate(const Resource* desired, Reason reason, void* data)
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
ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, const Resource* desired, Reason reason);
ControllerStatus ControllerApply(Controller* ctrl, const Resource* current, const ControllerAction action);
ControllerValidationResult ControllerValidate(Controller* ctrl, const Resource* current, Reason reason);
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
