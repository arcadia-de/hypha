#include "hypha/template_controller.h"

#include <errno.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/crc32.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/template_spec.h"
#include "hypha/validation_log.h"
#include "hypha/validation_result.h"

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  json_t* source = json_object_get(res->spec.doc, field);
  if (!source || !json_is_string(source))
    return false;

  Expander expander;
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

static inline const char* JsonTypeName(const json_type type) {
  switch (type) {
    case JSON_OBJECT:
      return "object";
    case JSON_ARRAY:
      return "array";
    case JSON_STRING:
      return "string";
    case JSON_INTEGER:
      return "integer";
    case JSON_REAL:
      return "real";
    case JSON_TRUE:
    case JSON_FALSE:
      return "boolean";
    case JSON_NULL:
      return "null";
    default:
      return "unknown";
  }
}

static inline char* ReadFileContents(const char* path) {
  FILE* in = fopen(path, "r");
  if (!in)
    return NULL;

  if (fseek(in, 0, SEEK_END) != 0) {
    fclose(in);
    return NULL;
  }

  const long size = ftell(in);
  if (size < 0) {
    fclose(in);
    return NULL;
  }
  rewind(in);

  char* buf = malloc((size_t)size + 1);
  if (!buf) {
    fclose(in);
    return NULL;
  }

  const size_t n = fread(buf, 1, (size_t)size, in);
  fclose(in);
  buf[n] = '\0';
  return buf;
}

static inline uint32_t crc32_file(const char* filename) {
  if (!filename)
    return 0;

  FILE* in = fopen(filename, "r");
  if (!in)
    return 0;

  if (fseek(in, 0, SEEK_END) != 0) {
    fclose(in);
    return 0;
  }

  const long size = ftell(in);
  if (size < 0) {
    fclose(in);
    return 0;
  }
  rewind(in);

  uint8_t bytes[size];
  const size_t n = fread(bytes, sizeof(uint8_t), (size_t)size, in);
  fclose(in);

  return HyphaCrc32C(bytes, n);
}

static inline char* ResolveTemplateSource(json_t* doc, bool* owned) {
  *owned = false;

  json_t* template = json_object_get(doc, "template");
  if (template)
    return (char*)json_string_value(template);

  json_t* template_file_field = json_object_get(doc, "templateFile");
  if (!template_file_field)
    return NULL;

  char* template_file = NULL;
  size_t template_file_len = 0;
  Expander expander;
  if (!ExpandStr(&expander, json_string_value(template_file_field), &template_file, &template_file_len))
    return NULL;

  char* contents = ReadFileContents(template_file);
  free(template_file);
  if (!contents)
    return NULL;

  *owned = true;
  return contents;
}

thread_local TemplateSpec template_spec;

static const char kTargetField[] = "target";

DEFINE_CONTROLLER_OBSERVE_FN(Template) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kTargetField, &template_spec.target, &template_spec.target_len)) {
    LOG_ERROR("failed to get `%s` field", kTargetField);
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Template) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!template_spec.target || template_spec.target_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` template_spec field", kTargetField);
    return false;
  }

  json_t* doc = desired->spec.doc;
  ASSERT(doc);

  json_t* dataField = json_object_get(doc, "data");
  if (dataField && !json_is_object(dataField) && !json_is_string(dataField)) {
    NewFailedValidationResult(log, desired, "expected template spec field `data` to be an object or string, got %s",
                              JsonTypeName(json_typeof(dataField)));
    return false;
  }

  json_t* templateField = json_object_get(doc, "template");
  if (templateField && !json_is_string(templateField)) {
    NewFailedValidationResult(log, desired, "expected template spec field `template` to be a string");
    return false;
  }
  if (templateField) {
    NewPassedValidationResult(log, desired, "Spec is valid");
    return true;
  }

  json_t* templateFileField = json_object_get(doc, "templateFile");
  if (templateFileField && !json_is_string(templateFileField)) {
    NewFailedValidationResult(log, desired, "expected template spec field `templateFile` to be a string");
    return false;
  }

  if (!templateFileField) {
    NewFailedValidationResult(log, desired, "template spec requires a `template` or `templateFile` field");
    return false;
  }

  char* templateFile = NULL;
  size_t templateFileLen = 0;
  Expander expander;
  if (!ExpandStr(&expander, json_string_value(templateFileField), &templateFile, &templateFileLen)) {
    NewFailedValidationResult(log, desired, "failed to expand `templateFile` field");
    return false;
  }

  struct stat template_file_stat;
  const bool exists = stat(templateFile, &template_file_stat) == 0;
  free(templateFile);
  if (!exists) {
    NewFailedValidationResult(log, desired, "`templateFile` does not exist");
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Template) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  json_t* doc = desired->spec.doc;
  ASSERT(doc);

  bool owned = false;
  char* source = ResolveTemplateSource(doc, &owned);
  if (!source) {
    PlannedAction* action = NewFailedPlannedAction(log, desired, "failed to resolve template source");
    ASSERT(action);
    return kFailedAction;
  }

  json_t* dataField = json_object_get(doc, "data");
  char* dataValue = NULL;
  bool data_owned = false;
  if (dataField) {
    if (json_is_object(dataField)) {
      dataValue = json_dumps(dataField, 0);
      data_owned = true;
    } else {
      dataValue = (char*)json_string_value(dataField);
    }
  }

  char* rendered = RenderTemplate(source, dataValue, false);
  if (owned)
    free(source);
  if (data_owned)
    free(dataValue);

  if (!rendered) {
    PlannedAction* action = NewFailedPlannedAction(log, desired, "failed to render template");
    ASSERT(action);
    return kFailedAction;
  }

  const uint32_t target_digest = crc32_file(template_spec.target);
  const uint32_t rendered_digest = HyphaCrc32C((const uint8_t*)rendered, strlen(rendered));
  free(rendered);

  if (rendered_digest == target_digest) {
    PlannedAction* action =
        NewNoPlannedAction(log, desired, "target `%s` already has the expected contents", template_spec.target);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action =
      NewUpdatePlannedAction(log, desired, "target `%s` does not have the expected contents", template_spec.target);
  ASSERT(action);
  return kUpdateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Template) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  json_t* doc = desired->spec.doc;
  ASSERT(doc);

  bool owned = false;
  char* source = ResolveTemplateSource(doc, &owned);
  if (!source) {
    LOG_ERROR("failed to resolve template source for `%s`", desired->info.name);
    return kStatusInternalError;
  }

  json_t* dataField = json_object_get(doc, "data");
  char* dataValue = NULL;
  bool data_owned = false;
  if (dataField) {
    if (json_is_object(dataField)) {
      dataValue = json_dumps(dataField, 0);
      data_owned = true;
    } else {
      dataValue = (char*)json_string_value(dataField);
    }
  }

  char* rendered = RenderTemplate(source, dataValue, false);
  if (owned)
    free(source);
  if (data_owned)
    free(dataValue);

  if (!rendered) {
    LOG_ERROR("failed to render template for `%s`", desired->info.name);
    return kStatusInternalError;
  }

  FILE* out = fopen(template_spec.target, "w");
  if (!out) {
    LOG_ERROR("failed to open `%s` for writing: %s", template_spec.target, strerror(errno));
    free(rendered);
    return kStatusInternalError;
  }

  const size_t len = strlen(rendered);
  const size_t written = fwrite(rendered, sizeof(char), len, out);
  fflush(out);
  fclose(out);
  free(rendered);

  if (written != len) {
    LOG_ERROR("short write to `%s`: wrote %zu of %zu bytes", template_spec.target, written, len);
    return kStatusInternalError;
  }

  AppliedAction* action =
      NewUpdateAction(ctx->log, desired, "`%s` rendered to `%s`", desired->info.name, template_spec.target);
  ASSERT(action);
  return kStatusOk;
}

DEFINE_CONTROLLER_STATUS_FN(Template) {
  const Resource* current = ctx->current;
  ASSERT(current);

  struct stat target_stat;
  if (stat(template_spec.target, &target_stat) != 0) {
    LOG_ERROR("target `%s` does not exist", template_spec.target);
    return kStatusInternalError;
  }

  return kStatusOk;
}

// Status only checks that `target` exists -- it doesn't look at whether the content is
// actually correct. Diff renders the template fresh (the same way Plan/Apply do) and
// compares it byte-for-byte against what's actually on disk, reporting the size difference
// or the first byte offset where they diverge. Genuinely more informative than Status, since
// Template is one of the few controllers here where the "expected" content is something we
// can regenerate and compare directly rather than just checking a checksum matches.
DEFINE_CONTROLLER_DIFF_FN(Template) {
  const Resource* observed = ctx->observed;
  ASSERT(observed);
  DeltaLog* dlog = ctx->log;
  ASSERT(dlog);

  json_t* doc = observed->spec.doc;
  ASSERT(doc);

  bool owned = false;
  char* source = ResolveTemplateSource(doc, &owned);
  if (!source) {
    NewNoDelta(dlog, "failed to resolve template source for `%s`", observed->info.name);
    return kStatusInternalError;
  }

  json_t* dataField = json_object_get(doc, "data");
  char* dataValue = NULL;
  bool data_owned = false;
  if (dataField) {
    if (json_is_object(dataField)) {
      dataValue = json_dumps(dataField, 0);
      data_owned = true;
    } else {
      dataValue = (char*)json_string_value(dataField);
    }
  }

  char* rendered = RenderTemplate(source, dataValue, false);
  if (owned)
    free(source);
  if (data_owned)
    free(dataValue);

  if (!rendered) {
    NewNoDelta(dlog, "failed to render template for `%s`", observed->info.name);
    return kStatusInternalError;
  }

  const size_t rendered_len = strlen(rendered);

  char* actual = ReadFileContents(template_spec.target);
  if (!actual) {
    NewNoDelta(dlog, "`%s` does not exist -- would be rendered with %zu bytes", template_spec.target, rendered_len);
    free(rendered);
    return kStatusInternalError;
  }

  const size_t actual_len = strlen(actual);
  if (rendered_len != actual_len) {
    NewNoDelta(dlog, "`%s` is %zu bytes, expected %zu bytes", template_spec.target, actual_len, rendered_len);
    free(rendered);
    free(actual);
    return kStatusInternalError;
  }

  size_t first_diff = 0;
  bool differs = false;
  for (; first_diff < rendered_len; first_diff++) {
    if (rendered[first_diff] != actual[first_diff]) {
      differs = true;
      break;
    }
  }

  free(rendered);
  free(actual);

  if (differs) {
    NewNoDelta(dlog, "`%s` differs from the expected rendered content starting at byte %zu", template_spec.target,
               first_diff);
    return kStatusInternalError;
  }

  return kStatusOk;
}

static const ControllerConfig kTemplateControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = TemplateObserve,
    .plan = TemplatePlan,
    .apply = TemplateApply,
    .validate = TemplateValidate,
    .diff = TemplateDiff,
    .status = TemplateStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};
static ResourceKind kTemplateKind = kInvalidResourceKind;

ResourceKind GetTemplateResourceKind() {
  return kTemplateKind;
}

Controller* NewTemplateController() {
  kTemplateKind = NewResourceKind(kTemplateControllerKindName);
  if (kTemplateKind == kInvalidResourceKind)
    return NULL;

  const char* aliases[2] = {
      "template",
      "templates",
  };
  return NewController(kTemplateKind, kTemplateControllerConfig, aliases, 2, NULL, NULL);
}
