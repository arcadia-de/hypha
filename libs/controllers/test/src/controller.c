#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/log.h"
#include "test_controller.h"

DEFINE_CONTROLLER_OBSERVE_FN(Test) {
  LOG_INFO("[observe] %s (thread %lu)", desired->id, (unsigned long)pthread_self());
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Test) {
  LOG_INFO("[plan] %s", desired->id);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Test) {
  LOG_INFO("[apply] %s action=%s (thread %lu", desired->id, ControllerActionToCString(action),
           (unsigned long)pthread_self());
  sleep(2);
  return kStatusOk;
}

static const ControllerConfig kTestControllerConfig = {
    .observe = TestObserve,
    .plan = TestPlan,
    .apply = TestApply,
};
DEFINE_NEW_CONTROLLER(Test, TEST_CONTROLLER_KIND);
