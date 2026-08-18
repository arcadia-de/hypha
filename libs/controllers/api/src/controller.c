#include "hypha/controller.h"

#include <stdlib.h>

#ifdef HYPHA_ENABLE_PROFILING
#include <tracy/tracy/TracyC.h>
#endif  // HYPHA_ENABLE_PROFILING

#include "hypha/log.h"

#ifndef MAX_NUMBER_OF_CONTROLLERS
#define MAX_NUMBER_OF_CONTROLLERS 32
#endif  // MAX_NUMBER_OF_CONTROLLERS

struct _Controller {
  const char* kind;
  ControllerConfig config;

  void* data;
  void (*free_data)(void*);
};

static uint32_t num_controllers = 0;
static Controller controllers[MAX_NUMBER_OF_CONTROLLERS];

#define BEGIN_FOREACH_CONTROLLER(Name)             \
  for (uint32_t i = 0; i < num_controllers; i++) { \
    Controller* Name = &controllers[i];

#define END_FOREACH_CONTROLLER }

const char* GetControllerKind(const Controller* ctrl) {
  return ctrl ? ctrl->kind : NULL;  // NOLINT(modernize-use-nullptr)
}

Controller* GetControllerForKind(const char* kind) {
  Controller* result = NULL;  // NOLINT(modernize-use-nullptr)
  if (!kind)
    goto finished;

  BEGIN_FOREACH_CONTROLLER(ctrl)
  if (IsControllerForKind(ctrl, kind)) {
    result = ctrl;
    goto finished;
  }
  END_FOREACH_CONTROLLER

finished:
  return result;
}

Controller* RegisterController(const char* kind, ControllerConfig config, void* data, void (*free_data)(void*)) {
  Controller* result = NULL;  // NOLINT(modernize-use-nullptr)

  if (num_controllers + 1 > MAX_NUMBER_OF_CONTROLLERS)
    goto finished;

  if (!kind)
    goto finished;

  result = &controllers[num_controllers];
  memset(result, 0, sizeof(Controller));
  num_controllers++;
  result->kind = strdup(kind);
  memmove(&result->config, &config, sizeof(ControllerConfig));
  result->data = data;
  result->free_data = free_data;
finished:
  return result;
}

bool VisitAllControllers(ControllerVisitFn vis, void* data) {
  bool success = false;

  BEGIN_FOREACH_CONTROLLER(ctrl)
  if (!vis(i, ctrl->kind, ctrl, data))
    goto finished;
  END_FOREACH_CONTROLLER

  success = true;
finished:
  return success;
}

uint32_t GetNumberOfRegisteredControllers() {
  return num_controllers;
}

Controller* GetControllerAt(const uint32_t i) {
  if (i > num_controllers)
    return NULL;  // NOLINT(modernize-use-nullptr)
  return &controllers[i];
}

void ControllerObserve(Controller* ctrl, const Resource* desired, Resource* res) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerObserve", 17);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  if (!desired || !res || !ctrl || !ctrl->config.observe)
    goto finished;

  ctrl->config.observe(desired, res, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
}

ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, const Resource* desired, Reason* reason) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerPlan", 14);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerAction result = kNoAction;

  if (!current || !desired || !ctrl || !ctrl->config.plan)
    goto finished;

  result = ctrl->config.plan(current, desired, reason, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return result;
}

ControllerStatus ControllerApply(Controller* ctrl, const Resource* current, const ControllerAction action) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerApply", 15);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.apply || action == kNoAction)
    goto finished;

  status = ctrl->config.apply(current, action, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return status;
}

ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerDestroy", 17);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.destroy)
    goto finished;

  status = ctrl->config.destroy(current, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return status;
}

ControllerValidationResult ControllerValidate(Controller* ctrl, const Resource* desired, Reason* reason) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerValidate", 18);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerValidationResult valid = kValidationkSkipped;

  if (!desired || !ctrl || !ctrl->config.validate) {
    goto finished;
  }

  valid = ctrl->config.validate(desired, reason, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return valid;
}

ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerDiff", 14);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.diff)
    goto finished;

  status = ctrl->config.diff(current, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return status;
}

ControllerStatus ControllerStat(Controller* ctrl, const Resource* current) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerStatus", 16);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.status)
    goto finished;

  status = ctrl->config.status(current, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return status;
}

ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerRollback", 18);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.rollback)
    goto finished;

  status = ctrl->config.rollback(current, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return status;
}

ControllerStatus ControllerNormalize(Controller* ctrl, const Resource* current) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerNormalize", 19);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.normalize)
    goto finished;

  status = ctrl->config.normalize(current, ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
  return status;
}

void ControllerInit(Controller* ctrl) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerInit", 14);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  if (!ctrl || !ctrl->config.init)
    goto finished;

  ctrl->config.init(ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
}

void ControllerDeInit(Controller* ctrl) {
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZone(ctx, 1);
  TracyCZoneName(ctx, "ControllerDeInit", 16);
  TracyCZoneText(ctx, GetControllerKind(ctrl), strlen(GetControllerKind(ctrl)));
#endif  // HYPHA_ENABLE_PROFILING
  if (!ctrl || !ctrl->config.deinit)
    goto finished;

  ctrl->config.deinit(ctrl->data);
finished:
#ifdef HYPHA_ENABLE_PROFILING
  TracyCZoneEnd(ctx);
#endif  // HYPHA_ENABLE_PROFILING
}
