#include "hypha/test_controller.h"

#include <bits/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/test_spec.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

static const char kSleepField[] = "sleep";
static inline void GetSleepField(const Resource* res, uint32_t* result) {
  ASSERT(res);
  ASSERT(res->spec.doc);
  json_t* sleep = json_object_get(res->spec.doc, kSleepField);
  if (!sleep || !json_is_integer(sleep)) {
    (*result) = kDefaultSleepSeconds;
    return;
  }

  (*result) = (uint32_t)json_integer_value(sleep);
}

thread_local TestSpec test_spec;

DEFINE_CONTROLLER_OBSERVE_FN(Test) {
  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Test) {
  ASSERT(ctx);
  ValidationLog* log = ctx->log;
  ASSERT(log);
  const Resource* desired = ctx->desired;
  ASSERT(desired);

  // TODO(@s0cks): const cast
  ValidationResult* vr =
      NewWarningValidationResult(log, (Resource*)desired, "This message always exists for Test resources");
  ASSERT(vr);
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Test) {
  ASSERT(ctx);
  Plan* pl = ctx->log;
  ASSERT(pl);
  const Resource* desired = ctx->desired;
  ASSERT(desired);

  GetSleepField(desired, &test_spec.sleep);
  // TODO(@s0cks): const cast
  PlannedAction* action = NewCreatePlannedAction(pl, (Resource*)desired, "Test resources are always created");
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Test) {
  const uint32_t total_seconds = test_spec.sleep;
  DLOG_INFO("sleeping for %u seconds", total_seconds);
  sleep(total_seconds);
  AppliedAction* action = NewCreateAction(ctx->log, ctx->desired, "Slept for %u seconds", total_seconds);
  ASSERT(action);
  return kStatusOk;
}

static const ControllerConfig kTestControllerConfig = {
    .observe = TestObserve,
    .validate = TestValidate,
    .plan = TestPlan,
    .apply = TestApply,
};
DEFINE_NEW_CONTROLLER(Test);
