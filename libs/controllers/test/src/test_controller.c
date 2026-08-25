#include "hypha/test_controller.h"

#include <bits/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/planner.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

DEFINE_CONTROLLER_OBSERVE_FN(Test) {
  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Test) {
  ValidationResult* vr = NewValidationResult(vlog);
  ASSERT(vr);
  vr->kind = kValidationWarning;
  vr->resource = desired;
  snprintf(vr->reason, HYPHA_REASON_MAX_LENGTH, "This message always exists for Test resources");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Test) {
  ASSERT(desired);
  PlannedAction* new_action = NewPlannedAction(pl);
  new_action->resource = desired;
  new_action->action = kCreateAction;
  clock_gettime(CLOCK_REALTIME, &new_action->timestamp);
  snprintf(new_action->reason, HYPHA_REASON_MAX_LENGTH, "Test resources are always created");
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Test) {
  sleep(2);
  return kStatusOk;
}

static const ControllerConfig kTestControllerConfig = {
    .observe = TestObserve,
    .validate = TestValidate,
    .plan = TestPlan,
    .apply = TestApply,
};
DEFINE_NEW_CONTROLLER(Test);
