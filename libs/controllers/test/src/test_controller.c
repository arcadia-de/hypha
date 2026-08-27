#include "hypha/test_controller.h"

#include <bits/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

static const uint32_t kDefaultSleepSeconds = 1;

typedef struct {
  uint32_t sleep;
} TestSpec;

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
  ValidationResult* vr = NewWarningValidationResult(vlog, desired, "This message always exists for Test resources");
  ASSERT(vr);
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Test) {
  ASSERT(desired);
  GetSleepField(desired, &test_spec.sleep);
  PlannedAction* action = NewCreatePlannedAction(pl, desired, "Test resources are always created");
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Test) {
  DLOG_INFO("sleeping for %d seconds", test_spec.sleep);
  sleep(test_spec.sleep);
  return kStatusOk;
}

static const ControllerConfig kTestControllerConfig = {
    .observe = TestObserve,
    .validate = TestValidate,
    .plan = TestPlan,
    .apply = TestApply,
};
DEFINE_NEW_CONTROLLER(Test);
