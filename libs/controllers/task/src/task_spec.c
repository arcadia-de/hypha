#include "hypha/task_spec.h"

#include <jansson.h>
#include <string.h>

#include "exec_spec.h"
#include "hypha/assertions.h"
#include "hypha/log.h"
#include "hypha/task_policy.h"

static inline void GetSpecPolicy(json_t* doc, TaskPolicy* result) {
  ASSERT(doc);
  json_t* policy = json_object_get(doc, "policy");
  if (!policy || !json_is_string(policy)) {
    (*result) = kDefaultTaskPolicy;
    return;
  }

  (*result) = ParseTaskPolicy(json_string_value(policy));
}

static inline void GetSpecTimeout(json_t* doc, TaskTimeout* result) {
  ASSERT(doc);
  json_t* timeout = json_object_get(doc, "timeout");
  if (!timeout || !json_is_integer(timeout)) {
    (*result) = kDefaultTaskTimeout;
    return;
  }

  (*result) = (TaskTimeout)json_integer_value(timeout);
}

static const char kExecField[] = "exec";
static inline void GetSpecExec(json_t* doc, ExecSpec* spec) {
  json_t* exec = json_object_get(doc, kExecField);
  if (!exec || !json_is_object(exec))
    return;
  ParseExecSpec(exec, spec);
}

static const char kCheckField[] = "check";
static inline void GetSpecCheck(json_t* doc, ExecSpec* spec) {
  json_t* exec = json_object_get(doc, kCheckField);
  if (!exec || !json_is_object(exec))
    return;
  ParseExecSpec(exec, spec);
}

static const char kRetryField[] = "retry";
static inline void GetSpecRetry(json_t* doc, TaskRetrySpec* result) {
  ASSERT(doc);
  result->policy = kDefaultRetryPolicy;
  result->attempts = 3;
  result->delay_ms = 0;

  json_t* retry = json_object_get(doc, kRetryField);
  if (!retry || !json_is_object(retry))
    return;

  json_t* policy = json_object_get(retry, "policy");
  if (policy && json_is_string(policy))
    result->policy = ParseRetryPolicy(json_string_value(policy));

  json_t* attempts = json_object_get(retry, "attempts");
  if (attempts && json_is_integer(attempts))
    result->attempts = (int32_t)json_integer_value(attempts);

  json_t* delay_ms = json_object_get(retry, "delay_ms");
  if (delay_ms && json_is_integer(delay_ms))
    result->delay_ms = (int32_t)json_integer_value(delay_ms);
}

static const char kSudoField[] = "sudo";
static inline void GetSpecSudo(json_t* doc, bool* result) {
  ASSERT(doc);
  (*result) = false;

  json_t* sudo = json_object_get(doc, kSudoField);
  if (sudo && json_is_boolean(sudo))
    (*result) = json_boolean_value(sudo);
}

bool ParseTaskSpec(json_t* doc, TaskSpec* spec) {
  ASSERT(doc);

  GetSpecPolicy(doc, &spec->policy);
  GetSpecTimeout(doc, &spec->timeout);
  GetSpecRetry(doc, &spec->retry);
  GetSpecSudo(doc, &spec->sudo);
  GetSpecExec(doc, &spec->exec);
  GetSpecCheck(doc, &spec->check);
  spec->has_check = spec->check.argv != NULL && spec->check.argv_len > 0;
  return true;
}
