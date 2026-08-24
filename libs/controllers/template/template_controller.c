#include "template_controller.h"

#include <stdio.h>

#include "hypha.h"
#include "hypha/crc32.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  Expander expander;
  json_t* source = json_object_get(res->spec.doc, field);
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

DEFINE_CONTROLLER_OBSERVE_FN(Template) {
  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Template) {
  char* target = NULL;
  size_t target_len = 0;
  if (!GetSpecField(desired, "target", &target, &target_len)) {
    ValidationResult* result =
        NewFailedValidationResult(vlog, desired, "failed to get `%s` field from template spec", "target");
    ASSERT(result);
    return false;
  }

  json_t* doc = desired->spec.doc;
  ASSERT(doc);

  json_t* dataField = json_object_get(doc, "data");
  if (dataField && !json_is_object(dataField)) {
    ValidationResult* result = NewFailedValidationResult(
        vlog, desired, "expected template spec field `%s` to be an object, got %s", "data", json_typeof(dataField));
    ASSERT(result);
    return false;
  }

  json_t* templateField = NULL;
  json_t* templateFileField = NULL;

  templateField = json_object_get(doc, "template");
  if (templateField && !json_is_string(templateField)) {
    ValidationResult* result =
        NewFailedValidationResult(vlog, desired, "expected template spec field `%s` to be a string", "template");
    ASSERT(result);
    return false;
  }
  if (templateField)
    goto finished;

  templateFileField = json_object_get(doc, "templateFile");
  if (templateFileField && !json_is_string(templateFileField)) {
    ValidationResult* result =
        NewFailedValidationResult(vlog, desired, "expected template spec field `%s` to be a string", "templateFile");
    ASSERT(result);
    return false;
  }

  if (templateFileField) {
    const char* templateFile = json_string_value(templateFileField);

    char* result = NULL;
    size_t result_len = 0;
    Expander expander;
    LOG_FATAL_IF(!ExpandStr(&expander, templateFile, &result, &result_len), "failed to expand templateFile field: %s",
                 templateFile);

    // TODO(@s0cks): check if result is a valid file

    free(result);
    goto finished;
  }

  {
    ValidationResult* result =
        NewFailedValidationResult(vlog, desired, "template spec requires a template or templateFile field");
    ASSERT(result);
    return false;
  }
finished:
  ValidationResult* result = NewPassedValidationResult(vlog, desired, "spec is valid");
  ASSERT(result);
  return true;
}

static inline uint32_t crc32_file(const char* filename) {
  if (!filename)
    return 0;

  FILE* in = fopen(filename, "r");
  if (!in)
    return 0;

  fseek(in, 0, SEEK_END);
  const uint64_t size = ftell(in);
  fseek(in, 0, SEEK_SET);

  uint8_t bytes[size];
  fread(bytes, sizeof(uint8_t), size, in);
  fclose(in);

  return crc32(bytes, size);
}

DEFINE_CONTROLLER_PLAN_FN(Template) {
  char* target = NULL;
  size_t target_len = 0;
  LOG_FATAL_IF(!GetSpecField(desired, "target", &target, &target_len),
               "failed to get `target` field from template spec");

  uint32_t target_digest = crc32_file(target);

  json_t* doc = desired->spec.doc;

  json_t* dataField = json_object_get(doc, "data");

  const char* dataValue = NULL;
  if (dataField) {
    if (json_is_object(dataField)) {
      dataValue = json_dumps(dataField, 0);
    } else if (json_is_string(dataField)) {
      dataValue = json_string_value(dataField);
    } else {
      LOG_FATAL("expected spec data field to be a string or object");
    }
  }

  char* rendered = NULL;
  json_t* template = json_object_get(doc, "template");
  if (template) {
    rendered = RenderTemplate((char*)json_string_value(template), (char*)dataValue, false);
    if (rendered) {
      uint32_t rendered_digest = crc32((const uint8_t*)rendered, strlen(rendered));
      if (rendered_digest == target_digest) {
        PlannedAction* action = NewNoPlannedAction(pl, desired, "template target file exists and has expected digest");
        ASSERT(action);
        return kNoAction;
      }

      PlannedAction* action = NewCreatePlannedAction(pl, desired, "template spec is valid");
      ASSERT(action);
      return kCreateAction;
    }

    goto finished;
  }

  json_t* templateFile = json_object_get(doc, "templateFile");
  if (templateFile) {}

finished:
  free(target);
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(Template) {
  json_t* doc = desired->spec.doc;

  char* target = NULL;
  size_t target_len = 0;
  LOG_FATAL_IF(!GetSpecField(desired, "target", &target, &target_len),
               "failed to get `target` field from template spec");

  json_t* dataField = json_object_get(doc, "data");
  const char* dataValue = NULL;
  if (dataField) {
    if (json_is_object(dataField)) {
      dataValue = json_dumps(dataField, 0);
    } else if (json_is_string(dataField)) {
      dataValue = json_string_value(dataField);
    } else {
      LOG_FATAL("expected spec data field to be a string or object");
    }
  }

  char* rendered = NULL;
  json_t* template = json_object_get(doc, "template");
  if (template) {
    rendered = RenderTemplate((char*)json_string_value(template), (char*)dataValue, false);
    // TODO(@s0cks): render to file
    LOG_INFO("rendered template:\n%s\n", rendered);

    FILE* out = fopen(target, "w");
    LOG_FATAL_IF(!out, "failed to open `%s` file for writing", target);
    size_t len = strlen(rendered);

    size_t written = fwrite(rendered, sizeof(char), len, out);
    LOG_WARN_IF(written != len, "failed to write template to file");

    fflush(out);
    fclose(out);
    return kStatusOk;
  }

  return kStatusInternalError;
}

static const ControllerConfig kTemplateControllerConfig = {
    .observe = TemplateObserve,
    .validate = TemplateValidate,
    .plan = TemplatePlan,
    .apply = TemplateApply,
};
DEFINE_NEW_CONTROLLER(Template, TEMPLATE_CONTROLLER_KIND);
