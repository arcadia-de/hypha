#ifndef HYPHA_CONTROLLER_H
#define HYPHA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdio.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/delta_log.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/resource_kind.h"
#include "hypha/state.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

// init controller
typedef void (*ControllerInitFn)(void* data);

// de-init controller
typedef void (*ControllerDeInitFn)(void* data);

#define DECLARE_CONTROLLER_FN(Name, RetType) typedef RetType (*Controller##Name##Fn)(Name##Context*, void*);

typedef struct {
  Resource* observed;
  StateEntry last;
} ObserveContext;
DECLARE_CONTROLLER_FN(Observe, ControllerStatus);

typedef struct {
  Resource* desired;
} NormalizeContext;
DECLARE_CONTROLLER_FN(Normalize, ControllerStatus);

typedef struct {
  const Resource* desired;
  ValidationLog* log;
} ValidateContext;
DECLARE_CONTROLLER_FN(Validate, bool);

typedef struct {
  const Resource* current;
  const Resource* desired;
  Plan* log;
} PlanContext;
DECLARE_CONTROLLER_FN(Plan, ControllerAction);

typedef struct {
  const Resource* current;
  const Resource* desired;
} StatusContext;
DECLARE_CONTROLLER_FN(Status, ControllerStatus);

typedef struct {
  const Resource* current;
} DestroyContext;
DECLARE_CONTROLLER_FN(Destroy, ControllerStatus);

typedef struct {
  const Resource* observed;
  const Resource* desired;
  DeltaLog log;
} DiffContext;
DECLARE_CONTROLLER_FN(Diff, ControllerStatus);

typedef struct {
  const Resource* current;
  const Resource* desired;
} RollbackContext;
DECLARE_CONTROLLER_FN(Rollback, ControllerStatus);

typedef struct {
  ControllerAction action;
  const Resource* current;
  Resource* desired;
  AppliedActionLog* log;
} ApplyContext;
DECLARE_CONTROLLER_FN(Apply, ControllerStatus);

// clang-format off
#define _DEFINE_CONTROLLER_FN(Name, State, RetType) \
  static inline RetType Name##State(State##Context* ctx, void* data)

#define DEFINE_CONTROLLER_INIT_FN(Name) \
  static inline void Name##Init(void* data)

#define DEFINE_CONTROLLER_DEINIT_FN(Name) \
  static inline void Name##DeInit(void* data)

#define DEFINE_CONTROLLER_OBSERVE_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Observe, ControllerStatus)

#define DEFINE_CONTROLLER_PLAN_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Plan, ControllerAction)

#define DEFINE_CONTROLLER_APPLY_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Apply, ControllerStatus)

#define DEFINE_CONTROLLER_DESTROY_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Destroy, ControllerStatus)

#define DEFINE_CONTROLLER_STATUS_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Status, ControllerStatus)

#define DEFINE_CONTROLLER_DIFF_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Diff, ControllerStatus)

#define DEFINE_CONTROLLER_VALIDATE_FN(Name) \
  _DEFINE_CONTROLLER_FN(Name, Validate, bool)
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
Controller* NewController(ResourceKind kind, ControllerConfig config, const char** aliases, size_t num_aliases,
                          void* data, void (*free_data)(void*));
ResourceKind GetControllerKind(const Controller* ctrl);

typedef bool (*ControllerAliasesVisitFn)(const uint64_t, char*, void*);
void VisitControllerAliases(Controller* ctrl, ControllerAliasesVisitFn fn, void* data);

void ControllerInit(Controller*);
void ControllerDeInit(Controller*);
ControllerStatus ControllerObserve(Controller* ctrl, Resource* current, StateEntry* last);
ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, Resource* desired, Plan* pl);
ControllerStatus ControllerApply(Controller* ctrl, Resource* desired, const ControllerAction action,
                                 AppliedActionLog* log);
bool ControllerValidate(Controller* ctrl, Resource* current, ValidationLog* vl);
ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current);
ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current);
ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current);
ControllerStatus ControllerStat(Controller* ctrl, const Resource* current);
ControllerStatus ControllerNormalize(Controller* ctrl, Resource* desired);

#define DECLARE_CONTROLLER(Name)                           \
  static const char k##Name##ControllerKindName[] = #Name; \
  ResourceKind Get##Name##ResourceKind();                  \
  Controller* New##Name##Controller();

#define DEFINE_NEW_CONTROLLER(Name)                                             \
  static ResourceKind k##Name##Kind = kInvalidResourceKind;                     \
  ResourceKind Get##Name##ResourceKind() {                                      \
    return k##Name##Kind;                                                       \
  }                                                                             \
  Controller* New##Name##Controller() {                                         \
    k##Name##Kind = NewResourceKind(k##Name##ControllerKindName);               \
    if (k##Name##Kind == kInvalidResourceKind)                                  \
      return NULL;                                                              \
    return NewController(k##Name##Kind, k##Name##ControllerConfig, NULL, NULL); \
  }

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_CONTROLLER_H
