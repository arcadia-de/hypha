#include "hypha/env.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/log.h"

extern char** environ;

void VisitAllEnvVars(EnvVarVisitor vis, void* data) {
  static const char kEnvVarDelims[] = "=";

  uint64_t idx = 0;
  for (char** env = environ; *env != NULL; env++) {
    char* str = (*env);
    char* key = strtok(str, kEnvVarDelims);
    char* value = strtok(NULL, kEnvVarDelims);
    if (!vis(idx, key, value, data))
      return;
  }
}

void AppendToEnvVar(const char* k, const char* value) {
  const char* current = getenv(k);
  if (current == NULL)
    return;
  const size_t new_size = snprintf(NULL, 0, "%s:%s", current, value) + 1;
  char* updated = (char*)malloc(new_size);
  LOG_FATAL_IF(!updated, "failed to create new env var for `%s`", k);
  const bool changed = setenv(k, updated, 1) != 0;
  LOG_ERROR_IF(!changed, "failed to set environment variable `%s` to `%s`", k, updated);
  DLOG_INFO_IF(changed, "new `%s`: %s", k, updated);
  free(updated);
}
