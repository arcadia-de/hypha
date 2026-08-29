#include "hypha/controller.h"

#include <stdlib.h>

#include "hypha/log.h"
#include "hypha/resource_kind.h"

#ifdef HYPHA_ENABLE_PROFILING

#include <tracy/tracy/TracyC.h>

#define BEGIN_CONTROLLER_FUNC(Name)          \
  TracyCZone(ctx, 1);                        \
  TracyCZoneName(ctx, #Name, strlen(#Name)); \
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));

#define END_CONTROLLER_FUNC TracyCZoneEnd(ctx);

#else

#define BEGIN_CONTROLLER_FUNC(Name)

#define END_CONTROLLER_FUNC

#endif  // HYPHA_ENABLE_PROFILING

struct _Controller {
  ResourceKind kind;
  ControllerConfig config;

  void* data;
  void (*free_data)(void*);
};

static const size_t kInitCap = 16;
static Controller* controllers = NULL;
static size_t controllers_len = 0;
static size_t controllers_cap = 0;

static inline bool EnsureCapacity(const size_t new_len) {
  if (new_len == 0 || new_len < controllers_cap)
    return false;

  const size_t new_cap = (controllers_cap + new_len) * 2;
  const size_t total_size = sizeof(Controller) * new_cap;
  Controller* new_controllers = (Controller*)realloc(controllers, total_size);
  if (!new_controllers)
    return false;

  controllers = new_controllers;
  controllers_cap = new_cap;
  return true;
}

Controller* NewController(ResourceKind kind, ControllerConfig config, void* data, void (*free_data)(void*)) {
  if (kind == kInvalidResourceKind)
    return NULL;

  if (!controllers) {
    const size_t total_size = sizeof(Controller) * kInitCap;
    Controller* new_controllers = (Controller*)malloc(total_size);
    if (!new_controllers)
      return NULL;
    memset(new_controllers, 0, total_size);
    controllers = new_controllers;
    controllers_len = 0;
    controllers_cap = kInitCap;
  } else {
    EnsureCapacity(controllers_len + 1);
  }

  Controller* ctrl = &controllers[controllers_len];
  controllers_len++;
  ctrl->kind = kind;
  ctrl->data = data;
  ctrl->free_data = free_data;
  memcpy(&ctrl->config, &config, sizeof(ControllerConfig));
  return ctrl;
}

ResourceKind GetControllerKind(const Controller* ctrl) {
  return ctrl ? ctrl->kind : kInvalidResourceKind;
}

Controller* GetControllerForKindName(const char* name) {
  ASSERT(name);
  ResourceKind kind = FindResourceKind(name);
  if (kind == kInvalidResourceKind)
    return NULL;

  return GetControllerForKind(kind);
}

Controller* GetControllerForKind(ResourceKind kind) {
  if (kind == kInvalidResourceKind)
    return NULL;

  for (size_t i = 0; i < controllers_len; i++) {
    Controller* ctrl = &controllers[i];
    ResourceKindInfo* info = GetResourceKindInfo(ctrl->kind);
    if (info->kind == kind)
      return ctrl;
  }

  return NULL;
}

bool VisitAllControllers(ControllerVisitFn vis, void* data) {
  bool success = false;

  for (size_t i = 0; i < controllers_len; i++) {
    Controller* ctrl = &controllers[i];
    ResourceKindInfo* info = GetResourceKindInfo(ctrl->kind);
    ASSERT(info);
    if (!vis(i, info->name, ctrl, data))
      goto finished;
  }

  success = true;
finished:
  return success;
}

uint64_t GetNumberOfRegisteredControllers() {
  return controllers_len;
}

Controller* GetControllerAt(const uint64_t i) {
  if (i > controllers_len)
    return NULL;  // NOLINT(modernize-use-nullptr)

  return &controllers[i];
}

ControllerStatus ControllerObserve(Controller* ctrl, const Resource* observed, Resource* desired) {
  BEGIN_CONTROLLER_FUNC(Observe);
  ControllerStatus status = kStatusInternalError;
  if (!desired || !desired || !ctrl || !ctrl->config.observe)
    goto finished;

  status = ctrl->config.observe(observed, desired, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, Resource* desired, Plan* pl) {
  BEGIN_CONTROLLER_FUNC(Plan);
  ControllerAction result = kNoAction;

  if (!current || !desired || !ctrl || !ctrl->config.plan)
    goto finished;

  result = ctrl->config.plan(current, desired, pl, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return result;
}

ControllerStatus ControllerApply(Controller* ctrl, const Resource* desired, const ControllerAction action) {
  BEGIN_CONTROLLER_FUNC(Apply);
  ControllerStatus status = kStatusOk;

  if (!desired || !ctrl || !ctrl->config.apply || action == kNoAction)
    goto finished;

  status = ctrl->config.apply(desired, action, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Destroy);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.destroy)
    goto finished;

  status = ctrl->config.destroy(current, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

bool ControllerValidate(Controller* ctrl, Resource* desired, ValidationLog* vl) {
  BEGIN_CONTROLLER_FUNC(Validate);
  bool valid = false;

  if (!desired || !ctrl || !ctrl->config.validate)
    goto finished;

  const Label* defaults = GetDefaultLabels();
  const size_t num_defaults = GetNumberOfDefaultLabels();
  if (defaults != NULL && num_defaults > 0)
    ResourcePushLabels(desired, defaults, num_defaults);

  valid = ctrl->config.validate(desired, vl, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return valid;
}

ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Diff);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.diff)
    goto finished;

  status = ctrl->config.diff(current, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerStat(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Status);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.status)
    goto finished;

  status = ctrl->config.status(current, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Rollback);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.rollback)
    goto finished;

  status = ctrl->config.rollback(current, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerNormalize(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Normalize);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.normalize)
    goto finished;

  status = ctrl->config.normalize(current, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

void ControllerInit(Controller* ctrl) {
  BEGIN_CONTROLLER_FUNC(Init);
  if (!ctrl || !ctrl->config.init)
    goto finished;

  ctrl->config.init(ctrl->data);
finished:
  END_CONTROLLER_FUNC;
}

void ControllerDeInit(Controller* ctrl) {
  BEGIN_CONTROLLER_FUNC(DeInit);
  if (!ctrl || !ctrl->config.deinit)
    goto finished;

  ctrl->config.deinit(ctrl->data);
finished:
  END_CONTROLLER_FUNC;
}

static inline void FreeController(Controller* ctrl) {
  if (!ctrl)
    return;

  if (ctrl->data && ctrl->free_data)
    ctrl->free_data(ctrl->data);
}

void FreeAllControllers() {
  if (!controllers)
    return;

  for (size_t i = 0; i < controllers_len; i++) {
    Controller* ctrl = &controllers[i];
    ASSERT(ctrl);
    FreeController(ctrl);
  }

  free(controllers);
  controllers = NULL;
  controllers_len = controllers_cap = 0;
}
