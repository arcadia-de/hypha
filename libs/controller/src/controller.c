#include "hypha/controller.h"

#include <stdlib.h>

#include "hypha/log.h"

#ifndef MAX_NUMBER_OF_CONTROLLERS
#define MAX_NUMBER_OF_CONTROLLERS 32
#endif  // MAX_NUMBER_OF_CONTROLLERS

struct _Controller {
  const char* kind;
  ControllerConfig config;
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

Controller* RegisterController(const char* kind, ControllerConfig config) {
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

  DLOG_DEBUG("registered %s controller", kind);
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
  if (!desired || !res || !ctrl || !ctrl->config.observe)
    return;
  ctrl->config.observe(desired, res);
}

ControllerAction ControllerPlan(Controller* ctrl, const Resource* current, const Resource* desired) {
  ControllerAction result = kNoAction;

  if (!current || !desired || !ctrl || !ctrl->config.plan)
    goto finished;

  result = ctrl->config.plan(current, desired);
finished:
  return result;
}

ControllerStatus ControllerApply(Controller* ctrl, const Resource* current, const ControllerAction action) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.apply || action == kNoAction)
    goto finished;

  status = ctrl->config.apply(current, action);
finished:
  return status;
}

ControllerStatus ControllerDestroy(Controller* ctrl, const Resource* current) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.destroy)
    goto finished;

  status = ctrl->config.destroy(current);
finished:
  return status;
}

ControllerStatus ControllerValidate(Controller* ctrl, const Resource* current) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.validate)
    goto finished;

  status = ctrl->config.validate(current);
finished:
  return status;
}

ControllerStatus ControllerDiff(Controller* ctrl, const Resource* current) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.diff)
    goto finished;

  status = ctrl->config.diff(current);
finished:
  return status;
}

ControllerStatus ControllerStat(Controller* ctrl, const Resource* current) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.status)
    goto finished;

  status = ctrl->config.status(current);
finished:
  return status;
}

ControllerStatus ControllerRollback(Controller* ctrl, const Resource* current) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.rollback)
    goto finished;

  status = ctrl->config.rollback(current);
finished:
  return status;
}

ControllerStatus ControllerNormalize(Controller* ctrl, const Resource* current) {
  ControllerStatus status = kStatusOk;

  if (!current || !ctrl || !ctrl->config.normalize)
    goto finished;

  status = ctrl->config.normalize(current);
finished:
  return status;
}

void ControllerInit(Controller* ctrl) {
  if (!ctrl || !ctrl->config.init)
    return;
  return ctrl->config.init();
}

void ControllerDeInit(Controller* ctrl) {
  if (!ctrl || !ctrl->config.deinit)
    return;
  return ctrl->config.deinit();
}
