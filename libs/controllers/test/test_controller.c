#include "test_controller.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/log.h"

DEFINE_CONTROLLER_OBSERVE_FN(Test) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Test) {
  snprintf(reason, HYPHA_REASON_MAX_LENGTH, "%s", "Test resources are always created");
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Test) {
  sleep(2);
  return kStatusOk;
}

static const ControllerConfig kTestControllerConfig = {
    .observe = TestObserve,
    .plan = TestPlan,
    .apply = TestApply,
};
DEFINE_NEW_CONTROLLER(Test, TEST_CONTROLLER_KIND);
