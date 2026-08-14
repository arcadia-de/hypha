#ifndef HYPHA_CONTROLLER_H
#define HYPHA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdio.h>

#include "hypha.h"
#include "hypha/resource.h"

#define FOR_EACH_CONTROLLER_STATUS(V) \
  V(Ok)                               \
  V(NoOp)                             \
  V(InvalidSpec)                      \
  V(NotFound)                         \
  V(Conflict)                         \
  V(Unsupported)                      \
  V(TransientError)                   \
  V(PermanentError)                   \
  V(InternalError)

// clang-format off
typedef enum {
#define DEFINE_STATUS(Name) kStatus##Name,
  FOR_EACH_CONTROLLER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
  kTotalNumberOfControllerStatuses,
} ControllerStatus;
// clang-format on

#define FOR_EACH_CONTROLLER_ACTION(V) \
  V(No)                               \
  V(Create)                           \
  V(Update)                           \
  V(Destroy)

// clang-format off
typedef enum {
#define DEFINE_ACTION(Name) k##Name##Action,
  FOR_EACH_CONTROLLER_ACTION(DEFINE_ACTION)
#undef DEFINE_ACTION
  kTotalNumberOfControllerActions,
} ControllerAction;
// clang-format on

static inline const char* ControllerActionToCString(const ControllerAction rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case k##Name##Action:       \
    return #Name;

    FOR_EACH_CONTROLLER_ACTION(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

// init controller
typedef void (*ControllerInitFn)();

// de-init controller
typedef void (*ControllerDeInitFn)();

// discover changes
typedef ControllerStatus (*ControllerObserveFn)(const Resource* desired, Resource* out);

// compute the change set
typedef ControllerAction (*ControllerPlanFn)(const Resource*, const Resource*);

// normalize the changes with standardized metadata
typedef ControllerStatus (*ControllerNormalizeFn)(const Resource*);

// execute the changes
typedef ControllerStatus (*ControllerApplyFn)(const Resource*, const ControllerAction);

// remove the resources cleanly
typedef ControllerStatus (*ControllerDestroyFn)(const Resource*);

// reject malformed specs before planning
typedef ControllerStatus (*ControllerValidateFn)(const Resource*);

// report the changes between current and planned
typedef ControllerStatus (*ControllerDiffFn)(const Resource*);

// report current state and drift
typedef ControllerStatus (*ControllerStatusFn)(const Resource*);

// rollback changes
typedef ControllerStatus (*ControllerRollbackFn)(const Resource*);

// clang-format off
#define DEFINE_CONTROLLER_OBSERVE_FN(Name) \
  static inline ControllerStatus Name##Observe(const Resource* desired, Resource* out)

#define DEFINE_CONTROLLER_PLAN_FN(Name) \
  static inline ControllerAction Name##Plan(const Resource* current, const Resource* desired)

#define DEFINE_CONTROLLER_APPLY_FN(Name) \
  static inline ControllerStatus Name##Apply(const Resource* desired, const ControllerAction action)

#define DEFINE_CONTROLLER_DESTROY_FN(Name) \
  static inline ControllerStatus Name##Destroy(const Resource* current)

#define DEFINE_CONTROLLER_STATUS_FN(Name) \
  static inline ControllerStatus Name##Status(const Resource* current)
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
Controller* RegisterController(const char* kind, ControllerConfig config);

const char* GetControllerKind(const Controller* ctrl);

void ControllerInit(Controller*);
void ControllerDeInit(Controller*);
void ControllerObserve(Controller* ctrl, const Resource* desired, Resource* res);
ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, const Resource* desired);
ControllerStatus ControllerApply(Controller* ctrl, const Resource* current, const ControllerAction action);
ControllerStatus ControllerValidate(Controller* ctrl, const Resource* current);
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

#define DEFINE_NEW_CONTROLLER(Name, Kind)                         \
  Controller* New##Name##Controller() {                           \
    return RegisterController((Kind), k##Name##ControllerConfig); \
  }

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_CONTROLLER_H
