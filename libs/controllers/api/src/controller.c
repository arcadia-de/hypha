#include "hypha/controller.h"

#include <stdlib.h>

#include "hypha/log.h"
#include "hypha/resource_kind.h"

#ifdef HYPHA_ENABLE_PROFILING

#include <tracy/tracy/TracyC.h>

#define BEGIN_CONTROLLER_FUNC(Name)          \
  DLOG_INFO("begin %s", #Name);              \
  TracyCZone(ctx, 1);                        \
  TracyCZoneName(ctx, #Name, strlen(#Name)); \
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));

#define END_CONTROLLER_FUNC TracyCZoneEnd(ctx);

#else

#define BEGIN_CONTROLLER_FUNC(Name) DLOG_INFO("begin %s", #Name);

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

ControllerStatus ControllerObserve(Controller* ctrl, Resource* observed, StateEntry* last) {
  ASSERT(ctrl);
  ASSERT(observed);
  BEGIN_CONTROLLER_FUNC(Observe);
  ControllerStatus status = kStatusInternalError;
  if (!ctrl->config.observe)
    goto no_op;

  ResourceInfo* observed_info = &observed->info;
  size_t num_labels = last->labels_len;
  if (num_labels > 0) {
    const size_t total_size = sizeof(Label) * num_labels;
    Label* labels = (Label*)malloc(total_size);
    memset(labels, 0, total_size);
    memcpy(labels, last->labels, total_size);
    observed_info->labels = labels;
    observed_info->labels_len = num_labels;
    observed_info->labels_cap = num_labels;
  }
  DLOG_INFO("decoded %zu/%zu labels", observed_info->labels_len, last->labels_len);

  const size_t num_annotations = last->annotations_len;
  if (num_annotations > 0) {
    const size_t total_size = sizeof(Annotation) * num_annotations;
    Annotation* annotations = (Annotation*)malloc(total_size);
    memset(annotations, 0, total_size);
    memcpy(annotations, last->annotations, total_size);
    observed->info.annotations = annotations;
    observed->info.annotations_len = observed->info.annotations_cap = num_annotations;
  }
  DLOG_INFO("decoded %zu annotations", observed->info.annotations_len);

  ObserveContext ctx = {
      .observed = observed,
  };
  status = ctrl->config.observe(&ctx, ctrl->data);
  goto finished;
no_op:
  status = kStatusNoOp;
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, Resource* desired, Plan* pl) {
  ASSERT(ctrl);
  ASSERT(current);
  ASSERT(desired);
  BEGIN_CONTROLLER_FUNC(Plan);
  ControllerAction result = kNoAction;

  if (!current || !desired || !ctrl || !ctrl->config.plan)
    goto finished;

  PlanContext ctx = {
      .current = current,
      .desired = desired,
      .log = pl,
  };
  result = ctrl->config.plan(&ctx, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return result;
}

ControllerStatus ControllerApply(Controller* ctrl, Resource* desired, const ControllerAction action,
                                 AppliedActionLog* log) {
  BEGIN_CONTROLLER_FUNC(Apply);
  ControllerStatus status = kStatusOk;

  if (!desired || !ctrl || !ctrl->config.apply || action == kNoAction)
    goto finished;

  ApplyContext ctx = {
      .desired = desired,
      .current = NULL,
      .action = action,
      .log = log,
  };
  status = ctrl->config.apply(&ctx, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current) {
  ASSERT(ctrl);
  ASSERT(current);
  BEGIN_CONTROLLER_FUNC(Destroy);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.destroy)
    goto finished;

  DestroyContext ctx = {
      .current = current,
  };
  status = ctrl->config.destroy(&ctx, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

bool ControllerValidate(Controller* ctrl, Resource* desired, ValidationLog* vl) {
  ASSERT(ctrl);
  ASSERT(desired);
  BEGIN_CONTROLLER_FUNC(Validate);
  bool valid = false;

  if (!ctrl->config.validate)
    goto no_op;
  ASSERT(ctrl->config.validate);

  ValidateContext ctx = {
      .log = vl,
      .desired = desired,
  };
  valid = ctrl->config.validate(&ctx, ctrl->data);
  goto finished;
no_op:
  valid = true;
finished:
  END_CONTROLLER_FUNC;
  return valid;
}

ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Diff);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.diff)
    goto finished;

  DiffContext ctx = {
      .current = current,
      .desired = NULL,
  };
  status = ctrl->config.diff(&ctx, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerStat(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Status);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.status)
    goto finished;

  StatusContext ctx = {
      .current = current,
      .desired = NULL,
  };
  status = ctrl->config.status(&ctx, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current) {
  BEGIN_CONTROLLER_FUNC(Rollback);
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.rollback)
    goto finished;

  RollbackContext ctx = {
      .current = current,
      .desired = NULL,
  };
  status = ctrl->config.rollback(&ctx, ctrl->data);
finished:
  END_CONTROLLER_FUNC;
  return status;
}

ControllerStatus ControllerNormalize(Controller* ctrl, Resource* desired) {
  ASSERT(ctrl);
  ASSERT(desired);
  BEGIN_CONTROLLER_FUNC(Normalize);
  ControllerStatus status = kStatusOk;
  if (!ctrl->config.normalize)
    goto no_op;

  NormalizeContext ctx = {
      .desired = desired,
  };
  status = ctrl->config.normalize(&ctx, ctrl->data);
  goto finished;
no_op:
  status = kStatusNoOp;
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
