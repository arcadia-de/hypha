#include "hypha.h"
#include "hypha/log.h"
#include "template_controller.h"

DEFINE_CONTROLLER_OBSERVE_FN(Template) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Template) {
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(Template) {
  return kStatusOk;
}

static const ControllerConfig kTemplateControllerConfig = {
    .observe = TemplateObserve,
    .plan = TemplatePlan,
    .apply = TemplateApply,
};
DEFINE_NEW_CONTROLLER(Template, TEMPLATE_CONTROLLER_KIND);
